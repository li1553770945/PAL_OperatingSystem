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
		for (int i=1,p=CurrentProcess->ID;i<MaxProcessCount;++i)
		{
			Process *tar=&Processes[(i+p)%MaxProcessCount];
			if (tar->stat==Process::S_Ready)
			{
				kout[Test]<<"Switch from "<<CurrentProcess->ID<<" to "<<tar->ID<<endl;
				tar->Run();
//				kout[Test]<<"Schedule return from "<<tar->ID<<"? to self "<<CurrentProcess->ID<<endl;
				break;
			}
			else if (tar->stat==Process::S_Quiting&&(tar->flags&Process::F_AutoDestroy))
				tar->Destroy();
		}
//		kout[Test]<<"ProcessManager::Schedule: Schedule complete, CurrentProcess "<<CurrentProcess->ID<<", TotalProcess "<<ProcessCount<<endl;
	}
	else ASSERT(CurrentProcess!=nullptr,"ProcessManager::Schedule: CurrentProcess is nullptr!");
}

void KernelThreadExit(int re)
{
	RegisterData a0=re,a7=SYS_Exit;
	asm volatile("ld a0,%0; ld a7,%1; ebreak"::"m"(a0),"m"(a7):"memory");
//	ProcessManager::Current()->Exit(re);//Multi cpu need get current of that cpu??
//	ProcessManager::Schedule();//Need improve...
	kout[Fault]<<"KernelThreadExit: Reached unreachable branch!"<<endl;
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
			return &Processes[i];
		}
	return nullptr;
}

ErrorType ProcessManager::FreeProcess(Process *proc)
{
	if (proc==CurrentProcess)
		kout[Fault]<<"ProcessManager::FreeProcess: proc==CurrentProcess with PID "<<proc->GetPID()<<"!"<<endl;
	Processes[proc->ID].stat=Process::S_None;
	--ProcessCount;
	return ERR_None;
}

ErrorType ProcessManager::Init()
{
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

ErrorType Process::Rest()
{
	if (stat==S_Running)
		ProcessManager::Schedule();//?
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
				cur->stat=S_Ready;
			stat=S_Running;
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
	ASSERT(stat!=S_Quiting,"Process::Exit: stat is S_Quiting(Exit twice)!");
	kout[Test]<<"Process::Exit: re "<<re<<" PID "<<ID<<endl;
	ReturnedValue=re;
	VMS->Leave();
	stat=S_Quiting;
	//Signal fa...
	return ERR_None;
}

ErrorType Process::Start(int (*func)(void*),void *funcdata)
{
	ASSERT(VMS!=nullptr,"Process::Start: VMS is nullptr!");
	ASSERT(Stack!=nullptr,"Process::Start: Stack is nullptr!");
	if (flags&F_Kernel)
	{
		context.ra  =(RegisterData)KernelThreadEntry;
    	context.sp  =(RegisterData)Stack+StackSize;
    	context.s[0]=(RegisterData)func;
    	context.s[1]=(RegisterData)funcdata;
    	context.s[2]=(RegisterData)((read_csr(sstatus)|SSTATUS_SPP|SSTATUS_SPIE)&~SSTATUS_SIE);
	}
	else
	{
		TrapFrame *tf=(TrapFrame*)(Stack+StackSize)-1;
		context.ra   =(RegisterData)UserThreadEntry;
    	context.sp   =(RegisterData)tf;
    	tf->reg.sp	 =(RegisterData)InnerUserProcessStackAddr+InnerUserProcessStackSize;
		tf->epc      =(RegisterData)func;
		tf->status   =(RegisterData)((read_csr(sstatus)|SSTATUS_SPIE)&~SSTATUS_SPP&~SSTATUS_SIE);//??
	}
	stat=S_Ready;
	return ERR_None;
}

ErrorType Process::Start(PtrInt addr)
{
	kout[Warning]<<"Process::Start is testing!"<<endl;
	return Start((int(*)(void*))addr,nullptr);
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

ErrorType Process::Start(TrapFrame *tf)//It is not a good way...
{
	MemcpyT<RegisterData>((RegisterData*)((TrapFrame*)(Stack+StackSize)-1),(const RegisterData*)tf,sizeof(TrapFrame)/sizeof(RegisterData));
	tf           =(TrapFrame*)(Stack+StackSize)-1;
	tf->epc+=4;
	context.ra   =(RegisterData)UserThreadEntry;
	context.sp   =(RegisterData)tf;
	return ERR_None;
}

ErrorType Process::CopyOthers(Process *src)
{
	CountingBase=src->CountingBase;
	RunningTime=src->RunningTime;
	StartedTime=src->StartedTime;
	SleepingTime=src->SleepingTime;
	WaitingTime=src->WaitingTime;
	//<<Set parent relationship...
	flags=src->flags;//??
	if (Name==nullptr)//??
		Name=strDump(src->Name);
	Namespace=src->Namespace;
	stat=src->stat;
	return ERR_None;
}

ErrorType Process::SetName(char *name)
{
	if (Name!=nullptr)
		Kfree(Name);
	Name=name;
	return ERR_None;
}

ErrorType Process::InitForKernelProcess0()
{
	stat=S_Running;
	RunningTime=0;
	CountingBase=RunningTime=StartedTime=SleepingTime=WaitingTime=0;
	Stack=bootstack;
	StackSize=KernelStackSize;
	fa=pre=nxt=child=nullptr;
	POS::MemsetT<RegisterData>((RegisterData*)&context,0,sizeof(context)/sizeof(RegisterData));
//	tf=nullptr;
	VMS=VirtualMemorySpace::Boot();
	flags=F_Kernel;
	Name="PAL_OperatingSystem BootProcess";
	Namespace=0;
	SemWaitingLink.SetData(this);
	SemWaitingTargetTime=0;
	return ERR_None;
}

ErrorType Process::Init(Uint64 _flags)
{
	ASSERT(stat==0,"Process::Init: stat is not 0");
	stat=S_Initing;
	RunningTime=0;
	CountingBase=RunningTime=StartedTime=SleepingTime=WaitingTime=0;
	Stack=nullptr;
	StackSize=0;
	fa=pre=nxt=child=nullptr;//<<Firstly,we don't use this...
	POS::MemsetT<RegisterData>((RegisterData*)&context,0,sizeof(context)/sizeof(RegisterData));
//	tf=nullptr;
	VMS=nullptr;
	flags=_flags;
	Name=nullptr;
	Namespace=0;
	SemWaitingLink.SetData(this);
	SemWaitingTargetTime=0;
	return ERR_None;
}

ErrorType Process::Destroy()
{
	kout[Test]<<"Process::Destroy: PID "<<ID<<endl;
	if (stat==S_None)
		return ERR_None;
	else if (POS::NotInSet(stat,S_Initing,S_Quiting))
		Exit(Exit_Destroy);
	VMS->Unref(this);
	VMS->TryDeleteSelf();
	VMS=nullptr;
	if (flags&F_GeneratedStack)
		Kfree(Stack);
	Stack=nullptr;
	SemWaitingLink.Remove();//What about Lock protect??
	stat=S_None;
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
			This->ThisProcess->stat=Process::S_Sleeping;
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
				nproc->Start(This->CurrentRequestingProcessTrapFrame);
				nproc->CopyOthers(proc);
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
		#ifdef QEMU
		write_csr(sstatus,read_csr(sstatus)|SSTATUS_SUM);
		#else
		write_csr(sstatus,read_csr(sstatus)&~SSTATUS_SUM);
		#endif
		MemcpyT<char>((char*)InnerUserProcessLoadAddr,(const char*)start,loadsize);
//		kout<<DataWithSize((void*)InnerUserProcessLoadAddr,loadsize)<<endl;
		#ifdef QEMU
		write_csr(sstatus,read_csr(sstatus)&~SSTATUS_SUM);
		#else
		write_csr(sstatus,read_csr(sstatus)|SSTATUS_SUM);
		#endif
		vms->Leave();
	}

	Process *proc=POS_PM.AllocProcess();
	proc->Init(flags);
	proc->SetStack(nullptr,KernelStackSize);
	proc->SetVMS(vms);
	proc->Start(InnerUserProcessLoadAddr);
	kout[Test]<<"CreateInnerUserImgProcess "<<(void*)start<<" "<<(void*)end<<" with PID "<<proc->GetPID()<<endl;
	return flags&Process::F_AutoDestroy?Process::UnknownPID:proc->GetPID();
}
