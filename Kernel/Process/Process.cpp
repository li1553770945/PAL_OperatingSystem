#include <Process/Process.hpp>
#include <Library/TemplateTools.hpp>
#include <Memory/PhysicalMemory.hpp>
#include <Trap/Interrupt.hpp>
#include <Error.hpp>
#include <Library/Kout.hpp>
#include <Riscv.h>
#include <SyscallID.hpp>
#include <Library/String/SysStringTools.hpp>
#include <Config.h>
#include <Process/Synchronize.hpp>
#include <Trap/Clock.h>
#include <File/FileSystem.hpp>
#include <File/FileNodeEX.hpp>

#include "../../Include/Process/Process.hpp"

using namespace POS;
	
Process ProcessManager::Processes[MaxProcessCount];
Process *ProcessManager::CurrentProcess=nullptr;
Uint32 ProcessManager::ProcessCount=0;
ProcessManager POS_PM;
ForkServerClass ForkServer;

void ProcessManager::Schedule()
{
	if (CurrentProcess!=nullptr&&ProcessCount>=2)
	{
//		kout[Test]<<"ProcessManager::Schedule: Start schedule, CurrentProcess "<<CurrentProcess->ID<<", TotalProcess "<<ProcessCount<<endl;
		int i,p;
		ClockTime minWaitingTarget=-1;
	RetrySchedule:
		for (i=1,p=CurrentProcess->ID;i<MaxProcessCount;++i)
		{
			Process *tar=&Processes[(i+p)%MaxProcessCount];
//			if (tar->GetPID()<ProcessCount)
//				kout[Debug]<<"tar "<<tar->GetPID()<<" stat "<<tar->GetStat()<<endl;
			if (tar->stat==Process::S_Sleeping&&NotInSet(tar->SemWaitingTargetTime,0ull,(Uint64)-1))
			{
				minWaitingTarget=minN(minWaitingTarget,tar->SemWaitingTargetTime);
				if (GetClockTime()>=tar->SemWaitingTargetTime)
					tar->SwitchStat(Process::S_Ready);
			}

			if (tar->stat==Process::S_Ready)
			{
//				kout[Test]<<"Switch from "<<CurrentProcess->ID<<" to "<<tar->ID<<endl;
				tar->Run();
//				kout[Test]<<"Schedule return from "<<tar->ID<<"? to self "<<CurrentProcess->ID<<endl;
				break;
			}
			else if (tar->stat==Process::S_Quiting&&(tar->flags&Process::F_AutoDestroy))
				tar->Destroy();
		}
//		kout[Test]<<"ProcessManager::Schedule: Schedule complete, CurrentProcess "<<CurrentProcess->ID<<", TotalProcess "<<ProcessCount<<endl;
		if (i==MaxProcessCount&&p!=0)
			if (minWaitingTarget!=-1)//??
				goto RetrySchedule;
//			else if (POS_PM.Current()->stat==Process::S_Ready)
//				DoNothing;
			else kout[Fault]<<"Scheduler failed to switch!"<<endl;
	}
	else ASSERT(CurrentProcess!=nullptr,"ProcessManager::Schedule: CurrentProcess is nullptr!");
}

extern bool OnTrap;
void KernelThreadExit(int re)
{
	RegisterData a0=re,a7=SYS_Exit;
	asm volatile("ld a0,%0; ld a7,%1; ebreak"::"m"(a0),"m"(a7):"memory");
//	ProcessManager::Current()->Exit(re);//Multi cpu need get current of that cpu??
//	ProcessManager::Schedule();//Need improve...
	kout[Fault]<<"KernelThreadExit: Reached unreachable branch!"<<endl;
}

void SwitchToUserStat()
{
	POS_PM.Current()->SwitchStat(Process::S_UserRunning);
}

void SwitchBackKernelStat()
{
	POS_PM.Current()->SwitchStat(Process::S_Running);
}

Process* ProcessManager::GetProcess(PID id)
{
	if (POS::InRange(id,1,MaxProcessCount-1))
		return &Processes[id];
	else return nullptr;
}

Process* ProcessManager::AllocProcess()
{
	for (int i=0;i<MaxProcessCount;++i)
		if (Processes[i].stat==Process::S_None)
		{
			++ProcessCount;
			Processes[i].SwitchStat(Process::S_Allocated);
			return &Processes[i];
		}
	return nullptr;
}

ErrorType ProcessManager::FreeProcess(Process *proc)
{
	if (proc==CurrentProcess)
		kout[Fault]<<"ProcessManager::FreeProcess: proc==CurrentProcess with PID "<<proc->GetPID()<<"!"<<endl;
	Processes[proc->ID].SwitchStat(Process::S_None);
	--ProcessCount;
	return ERR_None;
}

ErrorType ProcessManager::Init()
{
//	MemsetT<char>((char*)&Processes,0,sizeof(Processes));
	for (int i=0;i<MaxProcessCount;++i)
	{
		Processes[i].PM=this;
		Processes[i].ID=i;
	}
	Processes[0].InitForKernelProcess0();
	ProcessCount=1;
	CurrentProcess=&Processes[0];
	return ERR_None;
}

ErrorType ProcessManager::Destroy()
{
	for (int i=0;i<MaxProcessCount;++i)
		if (Processes[i].stat!=0)
			Processes[i].Destroy();
	return ERR_None;
}

ErrorType Process::SwitchStat(Uint32 tar)
{
//	kout[Debug]<<"Switch stat of "<<ID<<" from "<<stat<<" to "<<tar<<endl;
	ClockTime t=GetClockTime();
	ClockTime d=t-CountingBase;
	CountingBase=t;
	switch (stat)
	{
		case S_Allocated:
		case S_Initing:
			if (tar==S_Ready)
				StartedTime=t;
			break;
		case S_Ready:
			WaitingDuration+=d;
			break;
		case S_Running:
			RunningDuration+=d;
			break;
		case S_UserRunning:
			RunningDuration+=d;
			UserDuration+=d;
			break;
		case S_Sleeping:
			SleepingDuration+=d;
			break;
		case S_Quiting:
			break;
	}
	stat=tar;
	return ERR_None;
}

ErrorType Process::Rest()
{
	if (stat==S_Running)
	{
		if (OnTrap)
			ProcessManager::Schedule();
		else
		{
			RegisterData a7=SYS_Rest;
			asm volatile("ld a7,%0; ebreak"::"m"(a7):"memory");
		}
	}
//		ProcessManager::Schedule();//?
//		Process::Process0()->Run();//??
	else kout[Warning]<<"Process::Rest only current running Process is able to rest!"<<endl;
	return ERR_None;
}

ErrorType Process::Run()
{
//	ISAS
	{
		Process *cur=PM->Current();
		if (this!=cur)
		{
//			kout[Debug]<<"this "<<this<<" "<<GetPID()<<" Cur "<<PM->CurrentProcess->GetPID()<<endl;
			if (cur->stat==S_Running)//??
				cur->SwitchStat(S_Ready);
			SwitchStat(S_Running);
			PM->CurrentProcess=this;
			VMS->Enter();
			ProcessSwitchContext(&cur->context,&this->context);
			//Should not exist code here using local varible...
//			kout[Debug]<<"Mysp "<<this<<" "<<GetPID()<<" "<<(void*)this->context.sp<<" Cur "<<POS_PM.Current()->GetPID()<<endl;
		}
	}
	return ERR_None;
}

ErrorType Process::Exit(int re)
{
	ASSERTEX(stat!=S_Quiting,"Process::Exit: stat is S_Quiting(Exit twice),PID "<<ID);
	if (re!=Exit_Normal)
		kout[Warning]<<"Process "<<ID<<" exited with returned value "<<re<<endl;
	else kout[Test]<<"Process "<<ID<<" exited successfully."<<endl;
	ReturnedValue=re;
	VMS->Leave();
	if (!(flags&F_AutoDestroy)&&fa!=nullptr)
		fa->WaitSem->Signal();
	SwitchStat(S_Quiting);
	return ERR_None;
}

ErrorType Process::Start(int (*func)(void*),void *funcdata,PtrInt userStartAddr)
{
	ASSERT(VMS!=nullptr,"Process::Start: VMS is nullptr!");
	ASSERT(Stack!=nullptr,"Process::Start: Stack is nullptr!");
	if (flags&F_Kernel)
	{
		context.ra  =(RegisterData)KernelThreadEntry2;
    	context.sp  =(RegisterData)Stack+StackSize;
    	context.s[0]=(RegisterData)func;
    	context.s[1]=(RegisterData)funcdata;
//    	context.s[2]=(RegisterData)((read_csr(sstatus)|SSTATUS_SPP|SSTATUS_SPIE)&~SSTATUS_SIE);
	}
	else
	{
		TrapFrame *tf=(TrapFrame*)(Stack+StackSize)-1;
		context.ra   =(RegisterData)UserThreadEntry;
    	context.sp   =(RegisterData)tf;
    	context.s[0] =(RegisterData)func;
    	context.s[1] =(RegisterData)funcdata;
    	tf->reg.sp	 =(RegisterData)InnerUserProcessStackAddr/*??*/+InnerUserProcessStackSize-256;
		tf->epc      =(RegisterData)userStartAddr;
		tf->status   =(RegisterData)((read_csr(sstatus)|SSTATUS_SPIE)&~SSTATUS_SPP&~SSTATUS_SIE);//??
//		tf->status   =(RegisterData)((read_csr(sstatus)|SSTATUS_SPP|SSTATUS_SPIE)&~SSTATUS_SIE);//??
	}
	SwitchStat(S_Ready);
	return ERR_None;
}

ErrorType Process::SetVMS(VirtualMemorySpace *vms)
{
	if (VMS!=nullptr)
	{
		VMS->Unref(this);
		VMS->TryDeleteSelf();
	}
	VMS=vms;
	vms->Ref(this);
	return ERR_None;
}

ErrorType Process::SetStack(void *stack,Uint32 size)
{
	if (stack==nullptr)
	{
		flags|=F_GeneratedStack;
		stack=Kmalloc(size);
		if (stack==nullptr)
			return ERR_KmallocFailed;
	}
	POS::MemsetT<char>((char*)stack,0,size);
	Stack=stack;
	StackSize=size;
//	tf=(TrapFrame*)Stack-1;
	return ERR_None;
}

//ErrorType Process::CopyStackContext(Process *src)
//{
//	ASSERTEX(src,"Process::CopyStackContext "<<this<<" src "<<src<<" is nullptr!");
//	ASSERTEX(Stack==nullptr,"Process::CopyStackContext "<<this<<" Stack "<<Stack<<" is not nullptr!");
//	flags|=F_GeneratedStack;
//	StackSize=src->StackSize;
//	Stack=Kmalloc(StackSize);
//	if (Stack==nullptr)
//		return ERR_KmallocFailed;
//	kout[Debug]<<"Copy stack "<<Stack<<" from "<<src->Stack<<endl;
//	MemcpyT<char>((char*)Stack,(const char*)src->Stack,StackSize);
//	MemcpyT<RegisterData>((RegisterData*)&context,(const RegisterData*)&src->context,sizeof(Process::RegContext)/sizeof(RegisterData));
//	context.sp=src->context.sp-(PtrInt)src->Stack+(PtrInt)Stack;//??
//	kout[Debug]<<"sp "<<(void*)context.sp<<endl;
//	return ERR_None;
//}

ErrorType Process::Start(TrapFrame *tf,bool IsNew)//It is not a good way...
{
	if (!IsNew)
	{
		MemcpyT<RegisterData>((RegisterData*)((TrapFrame*)(Stack+StackSize)-1),(const RegisterData*)tf,sizeof(TrapFrame)/sizeof(RegisterData));
		tf=(TrapFrame*)(Stack+StackSize)-1;
		tf->epc+=4;
	}
	tf->reg.a0=0;//??
	context.ra=(RegisterData)UserThreadEntry;
	context.sp=(RegisterData)tf;
	context.s[0]=0;
	context.s[1]=0;
	return ERR_None;
}

ErrorType Process::CopyFileTable(Process *src)
{
	DestroyFileTable();
	for (FileHandle *p=src->FileTable[0];p;p=p->nxt)
	{
		FileHandle *q=p->Dump();
		if (q!=nullptr)
			q->BindToProcess(this,p->GetFD());
		else kout[Error]<<"Process::CopyFileTable failed to dumplicate FileHandle "<<p<<endl;
	}		
	return ERR_None;
}

ErrorType Process::CopyOthers(Process *src)
{
	CountingBase=src->CountingBase;
	RunningDuration=src->RunningDuration;
	StartedTime=src->StartedTime;
	SleepingDuration=src->SleepingDuration;
	WaitingDuration=src->WaitingDuration;
	UserDuration=src->UserDuration;
	SetFa(src->fa);//??
	flags=src->flags;//??
	if (Name!=nullptr)//??
		Name=strDump(src->Name);
	Namespace=src->Namespace;
	CurrentWorkDirectory=strDump(src->CurrentWorkDirectory);
	CopyFileTable(src);
//	stat=src->stat;
	return ERR_None;
}

ErrorType Process::SetName(char *name,bool outside)
{
	if (Name!=nullptr&&(flags&F_OutsideName))
		Kfree(Name);
	if (outside)
	{
		Name=name;
		flags|=F_OutsideName;
	}
	else
	{
		Name=strDump(name);
		flags&=~F_OutsideName;
	}
	return ERR_None;
}

ErrorType Process::SetFa(Process *_fa)
{
	ISAS
	{
		if ((flags&F_AutoDestroy)&&_fa!=nullptr)
			flags&=~F_AutoDestroy;//Remove the auto destroy flag for process with parent.
		if (fa!=nullptr)
		{
			if (fa->child==this)
				fa->child=nxt;
			else if (pre!=nullptr)
				pre->nxt=nxt;
			if (nxt!=nullptr)
				nxt->pre=pre;
			pre=nxt=fa=nullptr;
		}
		if (_fa!=nullptr)
		{
			fa=_fa;
			nxt=fa->child;
			fa->child=this;
			if (nxt!=nullptr)
				nxt->pre=this;
		}
	}
	return ERR_None;
}

Process* Process::GetQuitingChild(PID cid)
{
	ISAS for (Process *p=child;p;p=p->nxt)
		if (p->stat==S_Quiting&&InThisSet(cid,AnyPID,p->ID))
			return p;
	return nullptr;
}

ErrorType Process::SetCWD(const char *path)
{
	if (CurrentWorkDirectory)
		Kfree(CurrentWorkDirectory);
	CurrentWorkDirectory=strDump(path);
	return ERR_None;
}

FileHandle* Process::GetFileHandleFromFD(int fd)
{
	if (InRange(fd,0,7))
		return FileTable[fd];
	else for (FileHandle *fh=FileTable[0];fh;fh=fh->Nxt())
		if (fh->FD==fd)
			return fh;
	return nullptr;
}

ErrorType Process::InitFileTable()
{
	CurrentWorkDirectory=nullptr;
	MemsetT<FileHandle*>(FileTable,0,8);
	if (stdIO!=nullptr)
	{
		FileTable[0]=new FileHandle(stdIO,FileHandle::F_Read);
		FileTable[1]=new FileHandle(stdIO,FileHandle::F_Write);
		FileTable[2]=new FileHandle(stdIO,FileHandle::F_Write);//??
		FileTable[0]->FD=0;
		FileTable[1]->FD=1;
		FileTable[2]->FD=2;
		FileTable[0]->proc=this;
		FileTable[1]->proc=this;
		FileTable[2]->proc=this;
		FileTable[0]->NxtInsert(FileTable[1]);
		FileTable[1]->NxtInsert(FileTable[2]);
	}
	return ERR_None;
}

ErrorType Process::DestroyFileTable()
{
	for (FileHandle *p=FileTable[0],*q;p;p=q)
	{
		q=p->nxt;
		delete p;
	}
	return ERR_None;
}

ErrorType Process::InitForKernelProcess0()
{
	SwitchStat(S_Allocated);
	Init(F_Kernel);
	Stack=bootstack;
	StackSize=KernelStackSize;
	VMS=VirtualMemorySpace::Boot();
	SetName("PAL_OperatingSystem BootProcess",1);
	SwitchStat(S_Running);
	return ERR_None;
}

ErrorType Process::Init(Uint64 _flags)
{
	ASSERT(stat==S_Allocated,"Process::Init: stat is not S_Allocated");
	SwitchStat(S_Initing);
	CountingBase=GetClockTime();
	RunningDuration=StartedTime=SleepingDuration=WaitingDuration=UserDuration=0;
	Stack=nullptr;
	StackSize=0;
	fa=pre=nxt=child=nullptr;//<<Firstly,we don't use this...
	POS::MemsetT<RegisterData>((RegisterData*)&context,0,sizeof(context)/sizeof(RegisterData));
//	tf=nullptr;
	VMS=nullptr;
	flags=_flags;
	Name=nullptr;
	Namespace=0;
	SemWaitingLink.Init();
	SemWaitingLink.SetData(this);
	SemWaitingTargetTime=0;
	WaitSem=new Semaphore(0);
	Heap=nullptr;
	InitFileTable();
	CallingStack=nullptr;
	return ERR_None;
}

ErrorType Process::Destroy()
{
	kout[Test]<<"Process::Destroy: PID "<<ID<<endl;
	if (stat==S_None)
		return ERR_None;
	else if (POS::NotInSet(stat,S_Initing,S_Quiting))
		Exit(Exit_Destroy);
	while (child)
		child->Destroy();//??
	SetFa(nullptr);
	VMS->Unref(this);
	VMS->TryDeleteSelf();
	VMS=nullptr;
	if (flags&F_GeneratedStack)
		Kfree(Stack);
	Stack=nullptr;
	SemWaitingLink.Remove();//What about Lock protect??
	SemWaitingLink.Init();
	delete WaitSem;
	WaitSem=nullptr;
	SetName(nullptr,1);
	DestroyFileTable();
	if (CallingStack)
		kout[Warning]<<"DestroyProcess while CallingStack is not freed"<<endl;
	PM->FreeProcess(this);
	return ERR_None;
}

int ForkServerClass::ForkServerFunc(void *funcdata)
{
	ForkServerClass *This=(ForkServerClass*)funcdata;
	while (1)
	{
		Process *proc=This->CurrentRequestingProcess;
		if (proc==nullptr)
		{
			This->ThisProcess->SwitchStat(Process::S_Sleeping);
			ProcessManager::Schedule();
			kout[Warning]<<"ForkServer "<<This<<" PID "<<This->ThisProcess->GetPID()<<" reached unreachable branch???"<<endl;
		}
		else if (proc->IsKernelProcess())
			kout[Fault]<<"Fork is usable for kernel process yet!"<<endl;
		else
		{
//			kout[Debug]<<"THIS "<<This<<" "<<This->ThisProcess<<" Cur "<<POS_PM.Current()->GetPID()<<endl;
			kout[Test]<<"ForkServer start forking "<<proc<<" with PID "<<proc->GetPID()<<endl;
			ISAS//??
			{
				VirtualMemorySpace *nvms=KmallocT<VirtualMemorySpace>();
				nvms->Init();
				nvms->CreateFrom(proc->GetVMS());
				Process *nproc=POS_PM.AllocProcess();
				nproc->Init(0);
				nproc->SetVMS(nvms);
//				nproc->CopyStackContext(proc);
				nproc->SetStack(nullptr,proc->StackSize);
				nproc->Start(This->CurrentRequestingProcessTrapFrame,0);
				nproc->CopyOthers(proc);
				nproc->SwitchStat(Process::S_Ready);
				This->CurrentRequestingProcessTrapFrame->reg.a0=nproc->ID;
				This->CurrentRequestingProcess=nullptr;
				This->CurrentRequestingProcessTrapFrame=nullptr;
			}
			kout[Test]<<"ForkServer fork "<<proc->GetPID()<<" OK"<<endl;
			proc->Run();
		}
	}
	kout[Fault]<<"ForkServer "<<This<<" PID "<<This->ThisProcess->GetPID()<<" reached unreachable branch!"<<endl;
	return 0;
}

ErrorType ForkServerClass::RequestFork(Process *proc,TrapFrame *tf)
{
	if (CurrentRequestingProcess==nullptr)
	{
		CurrentRequestingProcess=proc;
		CurrentRequestingProcessTrapFrame=tf;
		ThisProcess->Run();
		return ERR_None;
	}
	else return ERR_BusyForking;
}

ErrorType ForkServerClass::Init()
{
	ISAS//??
	{
		lock.Init();
		CurrentRequestingProcess=nullptr;
		ThisProcess=POS_PM.GetProcess(CreateKernelThread(ForkServerFunc,this,0));
	}
	kout[Info]<<"ForkServer::Init "<<this<<" as Process "<<ThisProcess<<" with PID "<<ThisProcess->GetPID()<<" OK"<<endl;
	return ERR_None;
}

ErrorType ForkServerClass::Destroy()
{
	kout[Warning]<<"ForkServer::Destroy is not usable yet!"<<endl;
	return ERR_Todo;
}

PID CreateKernelThread(int (*func)(void*),void *funcdata,Uint64 flags)
{
	InterruptStackAutoSaverBlockController isas;
	Process *proc=POS_PM.AllocProcess();
	flags|=Process::F_Kernel;
	proc->Init(flags);
	proc->SetStack(nullptr,KernelStackSize);
	proc->SetVMS(VirtualMemorySpace::Kernel());
	if (!(flags&Process::F_AutoDestroy))
		proc->SetFa(POS_PM.Current());
	proc->Start(func,funcdata);
	return flags&Process::F_AutoDestroy?Process::UnknownPID:proc->GetPID();
}

PID CreateKernelProcess(int (*func)(void*),void *funcdata,Uint64 flags)
{
	kout[Warning]<<"CreateKernelProcess is not usable!"<<endl;
	return Process::InvalidPID;
}

PID CreateInnerUserImgProcess(PtrInt start,PtrInt end,Uint64 flags)
{
	InterruptStackAutoSaverBlockController isas;
	kout[Info]<<"CreateInnerUserImgProcess "<<(void*)start<<" "<<(void*)end<<" "<<(void*)flags<<endl;
	ASSERT(start<end,"CreateInnerUserImgProcess start>=end!");
	
	VirtualMemorySpace *vms=KmallocT<VirtualMemorySpace>();
	vms->Init();
	vms->Create(VirtualMemorySpace::VMS_CurrentTest);
	PtrInt loadsize=end-start,
		   loadstart=InnerUserProcessLoadAddr;
	VirtualMemoryRegion *vmr_bin=KmallocT<VirtualMemoryRegion>(),
						*vmr_stack=KmallocT<VirtualMemoryRegion>();
	vmr_bin->Init(loadstart,loadstart+loadsize,VirtualMemoryRegion::VM_RWX);
	vmr_stack->Init(InnerUserProcessStackAddr,InnerUserProcessStackAddr+InnerUserProcessStackSize,VirtualMemoryRegion::VM_USERSTACK);
	vms->InsertVMR(vmr_bin);
	vms->InsertVMR(vmr_stack);

	{//Test...
		vms->Enter();
		vms->EnableAccessUser();
		MemcpyT<char>((char*)InnerUserProcessLoadAddr,(const char*)start,loadsize);
		vms->DisableAccessUser();
		vms->Leave();
	}

	Process *proc=POS_PM.AllocProcess();
	proc->Init(flags);
	proc->SetStack(nullptr,KernelStackSize);
	proc->SetVMS(vms);
	if (!(flags&Process::F_AutoDestroy))
		proc->SetFa(POS_PM.Current());
	proc->Start(nullptr,nullptr,InnerUserProcessLoadAddr);
	kout[Test]<<"CreateInnerUserImgProcess "<<(void*)start<<" "<<(void*)end<<" with PID "<<proc->GetPID()<<endl;
	return flags&Process::F_AutoDestroy?Process::UnknownPID:proc->GetPID();
}
