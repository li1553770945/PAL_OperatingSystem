#include <Trap/Syscall.hpp>
#include <Library/Kout.hpp>
#include <Process/Process.hpp>
#include <Trap/Interrupt.hpp>
#include <Process/Synchronize.hpp>
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
//		kout[Debug]<<"ERRRRRRRRRRRRRRRRRRRRRRRRRRR "<<err<<endl;
		if (err==ERR_None)
//		{
//			Process *c=POS_PM.Current();
//			kout[Debug]<<"OKKKKKKKKKKKKKKKKKKKK "<<c->GetPID()<<endl;
//			register volatile RegisterData sp asm("sp");
//			kout[Debug]<<"CurrentSP "<<(void*)sp<<endl;
//			kout[Debug]<<"## "<<&err<<endl;
//			if (c==cur)
				return 0;
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
	vms->DisableAccesUser();
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
			PID re=proc->GetPID();
			proc->Destroy();
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
		case	SYS_clone		:
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
		case	SYS_uname		:
		case	SYS_sched_yeild	:
		case	SYS_gettimeofday:
		case	SYS_nanosleep	:
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
