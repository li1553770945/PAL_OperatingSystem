#include <Process/Process.hpp>
#include <Library/TemplateTools.hpp>
#include <Memory/PhysicalMemory.hpp>
#include <Trap/Interrupt.h>
#include <Error.hpp>
#include <Library/Kout.hpp>
#include <Riscv.h>

using namespace POS;
	
Process ProcessManager::Processes[MaxProcessCount];
Process *ProcessManager::CurrentProcess=nullptr;
Uint32 ProcessManager::ProcessCount=0;
ProcessManager POS_PM;

void ProcessManager::Schedule()
{
	if (CurrentProcess!=nullptr&&ProcessCount>=2)
	{
		kout[Test]<<"ProcessManager::Schedule: Start schedule, CurrentProcess "<<CurrentProcess->ID<<", TotalProcess "<<ProcessCount<<endl;
		for (int i=1,p=CurrentProcess->ID;i<MaxProcessCount;++i)
		{
			Process *tar=&Processes[(i+p)%MaxProcessCount];
			if (tar->stat==Process::S_Ready)
			{
				kout[Test]<<"Switch to "<<tar->ID<<endl;
				tar->Run();
				break;
			}
			else if (tar->stat==Process::S_Quiting&&(tar->flags&Process::F_AutoDestroy))
				tar->Destroy();
		}
		kout[Test]<<"ProcessManager::Schedule: Schedule complete."<<endl;
	}
	else ASSERT(CurrentProcess!=nullptr,"ProcessManager::Schedule: CurrentProcess is nullptr!");
}

void KernelThreadExit(int re)
{
	ProcessManager::Current()->Exit(re);//Multi cpu need get current of that cpu??
	ProcessManager::Schedule();
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
	return ERR_None;
}

ErrorType Process::Rest()
{
	if (stat==S_Running)
	{
		using namespace POS;
		stat=S_Ready;//??
	}
	return ERR_None;
}

ErrorType Process::Run()
{
//	ISAS
	{
		Process *cur=PM->Current();
		if (this!=cur)
		{
			cur->Rest();
			stat=S_Running;
			PM->CurrentProcess=this;
			VMS->Enter();
			ProcessSwitchContext(&cur->context,&this->context);
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
	else kout[Fault]<<"Process::Start: Uncompleted function of start as non-kernel process!"<<endl;
	stat=S_Ready;
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

ErrorType Process::SetName(char *name)
{
	if (Name!=nullptr)
		Kfree(Name);
	Name=name;
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
	stat=S_None;
	PM->FreeProcess(this);
	return ERR_None;
}
