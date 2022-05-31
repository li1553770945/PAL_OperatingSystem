#include <Trap/Syscall.hpp>
#include <Library/Kout.hpp>
#include <Process/Process.hpp>
#include <Trap/Interrupt.hpp>
#include <Process/Synchronize.hpp>
#include <Library/String/StringTools.hpp>
using namespace POS;

inline void Syscall_Putchar(char ch)
{Putchar(ch);}

inline char Syscall_Getchar()
{return Getchar();}

inline char Syscall_Getputchar()
{return Getputchar();}

void Syscall_Exit(int re)
{
	Process *cur=POS_PM.Current();
	cur->Exit(re);
	POS_PM.Schedule();
	kout[Fault]<<"Syscall_Exit: Reached unreachable branch!"<<endl;
}

PID Syscall_Fork(TrapFrame *tf)
{
	Process *cur=POS_PM.Current();
	while (1)
	{
		ErrorType err=ForkServer.RequestFork(cur,tf);
		if (err==ERR_None)
//			Process *c=POS_PM.Current();
//			if (c==cur)
				return tf->reg.a0; 
//			else
//			{
//				kout[Fault]<<"???"<<endl;
//			}
//			return cur==c?0:c->GetPID();//??
//		}
		else if (err!=ERR_BusyForking)
			return Process::InvalidPID;
//		kout[Fault]<<"Request again??"<<endl;
	}
}

inline PID Syscall_Clone(TrapFrame *tf,Uint64 flags,void *stack,PID ppid,Uint64 tls,PID cid)
{
	if (ppid!=0||tls!=0||cid!=0)
		kout[Warning]<<"Syscall_Clone: Currently not support ppid,tls,cid parameter!"<<endl;
	constexpr Uint64 SIGCHLD=17;
	PID re=-1;
	ISAS
	{
		Process *cur=POS_PM.Current();
		Process *nproc=POS_PM.AllocProcess();
		ASSERTEX(nproc,"Syscall_Clone: Failed to create process!");
		nproc->Init(flags&SIGCHLD?0:Process::F_AutoDestroy);
		re=nproc->GetPID();
		nproc->SetStack(nullptr,cur->StackSize);
		if (stack==nullptr)//Aka fork
		{
			VirtualMemorySpace *nvms=KmallocT<VirtualMemorySpace>();
			nvms->Init();
			nvms->CreateFrom(cur->GetVMS());
			nproc->SetVMS(nvms);
			nproc->Start(tf,0);
		}
		else//Aka create thread 
		{
			nproc->SetVMS(cur->GetVMS());
			TrapFrame *ntf=(TrapFrame*)(nproc->Stack+nproc->StackSize)-1;
			MemcpyT<RegisterData>((RegisterData*)ntf,(const RegisterData*)tf,sizeof(TrapFrame)/sizeof(RegisterData));
			ntf->epc+=4;
			ntf->reg.sp=(RegisterData)stack;
			nproc->Start(ntf,1);
		}
		nproc->CopyOthers(cur);
		if (flags&SIGCHLD)
			nproc->SetFa(cur);
		nproc->stat=Process::S_Ready;
	}
	return re;
}

inline PID Syscall_GetPID()
{return POS_PM.Current()->GetPID();}

inline RegisterData Syscall_Write(int fd,void *dst,Uint64 size)
{
//	kout[Warning]<<"Currently Syscall_Write can only use fd == 1!"<<endl;
	if (fd!=1)
	{
		kout[Error]<<"Currently Syscall_Write can only use fd == 1, while fd is "<<fd<<endl;
		return (RegisterData)-1;
	}
	Process *proc=POS_PM.Current();
	VirtualMemorySpace *vms=proc->GetVMS();
	vms->EnableAccessUser();
	for (int i=0;i<size;++i)
		Putchar(*(char*)(dst+i));
	vms->DisableAccessUser();
	return size;
}

inline PID Syscall_Wait4(PID cid,int *status,int options)
{
	constexpr int WNOHANG=1;
	Process *proc=POS_PM.Current();
	while (1)//??
	{
		Process *child=proc->GetQuitingChild(cid==-1?Process::AnyPID:cid);
		if (child!=nullptr)
		{
			PID re=child->GetPID();
			if (status!=nullptr)
			{
				VirtualMemorySpace::EnableAccessUser();
				*status=child->GetReturnedValue();
				VirtualMemorySpace::DisableAccessUser();
			}
			child->Destroy();
			return re;
		}
		else if (options&WNOHANG)
			return -1;
		else proc->GetWaitSem()->Wait();
	}
}

inline PID Syscall_GetPPID()
{
	Process *proc=POS_PM.Current();
	Process *fa=proc->GetFa();
	if (fa==nullptr)
		return Process::InvalidPID;
	else return fa->GetPID();
}

inline RegisterData Syscall_Uname(RegisterData p)
{
	struct utsname
	{
		char sysname[65];
		char nodename[65];
		char release[65];
		char version[65];
		char machine[65];
		char domainname[65];
	}*u=(utsname*)p;
	Process *proc=POS_PM.Current();
	VirtualMemorySpace *vms=proc->GetVMS();
	vms->EnableAccessUser();
	strCopy(u->sysname,"PAL_OperatingSystem");
	strCopy(u->nodename,"PAL_OperatingSystem");
	strCopy(u->release,"Debug");
	strCopy(u->version,"0.3");
	strCopy(u->machine,"Riscv64");
	strCopy(u->domainname,"PAL");
	vms->DisableAccessUser();
	return 0;
}

inline RegisterData Syscall_sched_yeild()
{
	Process *proc=POS_PM.Current();
	proc->Rest();
	return 0;
}

inline RegisterData Syscall_gettimeofday(RegisterData _tv)
{
	struct timeval
	{
		int tv_sec;
		int tv_usec;
	}*tv=(timeval*)_tv;
	//<<Improve timer related to 1700.
	VirtualMemorySpace::EnableAccessUser();
	ClockTime t=GetClockTime();
	tv->tv_sec=t/Timer_1s;
	tv->tv_usec=t%Timer_1s/Timer_1us;
	VirtualMemorySpace::DisableAccessUser();
	return 0;
}

inline RegisterData Syscall_nanosleep(RegisterData _req,RegisterData _rem)
{
	struct timespec
	{
		int tv_sec;
		int tv_nsec;
	}
	*req=(timespec*)_req,
	*rem=(timespec*)_rem;
	VirtualMemorySpace::EnableAccessUser();
	Semaphore sem(0);
	sem.Wait(req->tv_sec*Timer_1s+
			 req->tv_nsec/1000000*Timer_1ms+
			 req->tv_nsec%1000000/1000*Timer_1us+
			 req->tv_nsec%1000*Timer_1ns);
	rem->tv_sec=rem->tv_nsec=0;//??
	VirtualMemorySpace::DisableAccessUser();
	return 0;
}

ErrorType TrapFunc_Syscall(TrapFrame *tf)
{
	InterruptStackAutoSaverBlockController isas;//??
//	kout[Test]<<"Syscall "<<tf->reg.a7<<" | "<<tf->reg.a0<<" "<<tf->reg.a1<<" "<<tf->reg.a2<<" "<<tf->reg.a3<<" "<<tf->reg.a4<<" "<<tf->reg.a5<<endl;
	switch (tf->reg.a7)
	{
		case SYS_Putchar:
			Syscall_Putchar(tf->reg.a0);
			break;
		case SYS_Getchar:
			tf->reg.a0=Syscall_Getchar();
			break;
		case SYS_Getputchar:
			tf->reg.a0=Syscall_Getputchar();
			break;
		case SYS_Exit:
			Syscall_Exit(tf->reg.a0);
			break;
		case SYS_Fork:
			Syscall_Fork(tf);
			break;
		case SYS_GetPID:
			tf->reg.a0=Syscall_GetPID();
			break;
		
		case	SYS_getcwd		:
		case	SYS_pipe2		:
		case	SYS_dup			:
		case	SYS_dup3		:
		case	SYS_chdir		:
		case	SYS_openat		:
		case	SYS_close		:
		case	SYS_getdents64	:
		case	SYS_read		:
			goto Default;
		case	SYS_write		:
			tf->reg.a0=Syscall_Write(tf->reg.a0,(void*)tf->reg.a1,tf->reg.a2);
			break;
		case	SYS_linkat		:
		case	SYS_unlinkat	:
		case	SYS_mkdirat		:
		case	SYS_umount2		:
		case	SYS_mount		:
		case	SYS_fstat		:
			goto Default;
		case	SYS_clone		:
			tf->reg.a0=Syscall_Clone(tf,tf->reg.a0,(void*)tf->reg.a1,tf->reg.a2,tf->reg.a3,tf->reg.a4);
			break;
		case	SYS_execve		:
			goto Default;
		case	SYS_wait4		:
			tf->reg.a0=Syscall_Wait4(tf->reg.a0,(int*)tf->reg.a1,tf->reg.a2);
			break;
		case	SYS_exit		:
			Syscall_Exit(tf->reg.a0);
			break;
		case	SYS_getppid		:
			tf->reg.a0=Syscall_GetPPID();
			break;
		case	SYS_getpid		:
			tf->reg.a0=Syscall_GetPID();
			break;
		case	SYS_brk			:
		case	SYS_munmap		:
		case	SYS_mmap		:
		case	SYS_times		:
			goto Default;
		case	SYS_uname		:
			tf->reg.a0=Syscall_Uname(tf->reg.a0);
			break;
		case	SYS_sched_yeild	:
			tf->reg.a0=Syscall_sched_yeild();
			break;
		case	SYS_gettimeofday:
			tf->reg.a0=Syscall_gettimeofday(tf->reg.a0);
			break;
		case	SYS_nanosleep	:
			tf->reg.a0=Syscall_nanosleep(tf->reg.a0,tf->reg.a1);
			break;
		default:
		Default:
		{
			Process *cur=POS_PM.Current();
			if (cur->IsKernelProcess())
				kout[Fault]<<"TrapFunc_Syscall: Unknown syscall "<<tf->reg.a7<<" from kernel process "<<cur->GetPID()<<"!"<<endl;
			else
			{
				kout[Error]<<"TrapFunc_Syscall: Unknown syscall "<<tf->reg.a7<<" from user process "<<cur->GetPID()<<"!"<<endl;
				cur->Exit(Process::Exit_BadSyscall);
				POS_PM.Schedule();
			}
			break;
		}
	}
	tf->epc+=4;
	return ERR_None;
}
