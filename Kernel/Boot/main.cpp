#include <Boot/SystemInfo.h>
#include <Trap/Clock.h>
#include <Trap/Interrupt.h>
#include <Trap/Trap.hpp>
#include <Library/Kout.hpp>
#include <Process/Process.hpp>
#include <Memory/VirtualMemory.hpp>

extern "C"
{
	void *__dso_handle=0;
	void *__cxa_atexit=0;
	void *_Unwind_Resume=0;
	void *__gxx_personality_v0=0;
};//??

using namespace POS;

int KernelThreadTest(void *data)
{
	using namespace POS;
	kout<<"KernelThreadTest: data "<<data<<endl;
	for (int i=0;i<5;++i)
	{
		kout<<"KernelThreadTest: "<<i<<endl;
		for (int j=0;j<=1e8;++j)
			;
	}
	kout<<"KernelThreadTest: Exit"<<endl;
	return 0;
}

int KernelThreadTest2(void *data)
{
	using namespace POS;
	kout<<"KernelThreadTest2: data "<<data<<endl;
	for (char i='A';i<='G';++i)
	{
		kout<<"KernelThreadTest2: "<<i<<endl;
		for (int j=0;j<=1e8;++j)
			;
	}
	kout<<"KernelThreadTest2: Exit"<<endl;
	return 0;
}

void TestFuncs()
{
	if (1)
	{
		Process *proc=POS_PM.AllocProcess();
		proc->Init(Process::F_Kernel|Process::F_AutoDestroy);
		proc->SetStack(nullptr,KernelStackSize);
		proc->SetVMS(VirtualMemorySpace::Kernel());
		proc->Start(KernelThreadTest,proc);
	}
	
	if (1)
	{
		Process *proc=POS_PM.AllocProcess();
		proc->Init(Process::F_Kernel|Process::F_AutoDestroy);
		proc->SetStack(nullptr,KernelStackSize);
		proc->SetVMS(VirtualMemorySpace::Kernel());
		proc->Start(KernelThreadTest2,proc);
	}
	
	if (1)
	{
		for (int i=10000;i<=20000;i+=100)
			(*(char*)i)=i;
	}
	
	if (1)
	{
		kout[Info]<<"   Kout Info Text"<<endl;
		kout[Warning]<<"Kout Warning Text"<<endl;
		kout[Error]<<"  Kout Error Text"<<endl;
		kout[Debug]<<"  Kout Debut Text"<<endl;
		kout[Test]<<"   Kout Test Text"<<endl;
	}
}

int main()
{
	PrintSystemInfo();
	POS_InitClock();
	POS_InitTrap();
	POS_PMM.Init();
	VirtualMemorySpace::InitStatic();
	POS_PM.Init();
	InterruptEnable();
	
	TestFuncs();
	
	//Below do nothing...
	auto Sleep=[](int n){while (n-->0);};
	auto DeadLoop=[Sleep](const char *str)
	{
		while (1)
			Sleep(1e8),
			kout<<DarkGray<<str<<Reset;
	};
	DeadLoop(".");
	return 0;
}
