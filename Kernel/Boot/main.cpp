#include <Boot/SystemInfo.h>
#include <Trap/Clock.h>
#include <Trap/Interrupt.hpp>
#include <Trap/Trap.hpp>
#include <Library/Kout.hpp>
#include <Process/Process.hpp>
#include <Memory/VirtualMemory.hpp>
#include <Resources.hpp>
#include <File/FileSystem.hpp>

extern "C"
{
	void *__dso_handle=0;
	void *__cxa_atexit=0;
	void *_Unwind_Resume=0;
	void *__gxx_personality_v0=0;
};//??

void* operator new(long unsigned int size)
{
	return Kmalloc(size);
}

void operator delete(void *p,long unsigned int size)
{
	Kfree(p);
}

using namespace POS;

class A
{
	public:
		int x=-1;
		
		A(int _x):x(_x)
		{
			kout<<"Construct A "<<x<<endl;
		}
		
		A()
		{
			kout<<"Construct A "<<x<<endl;
		}
		
		~A()
		{
			kout<<"Deconstruct A "<<x<<endl;
		}
};

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
	if (0)
	{
		char ch=Getchar();
		kout<<"ch "<<(int)ch<<endl;
	}
	
	if (0)
	{
		char s[100];
		int len=Getline(s,100);
		s[minN(len,99)]=0;
		kout<<"User input: "<<s<<endl;
	}
	
	if (0)
	{
		kout[Test]<<"KmallocTest..."<<endl;
		int *p=(int*)Kmalloc(4096);
		kout[Test]<<"KmallocTest OK."<<endl;
		kout[Test]<<"KfreeTest..."<<endl;
		Kfree(p);
		kout[Test]<<"KfreeTest OK."<<endl;
	}
	
	if (0) CreateKernelThread(KernelThreadTest,nullptr);
	if (1) CreateKernelThread(KernelThreadTest2,nullptr);
	if (1) CreateInnerUserImgProcess((PtrInt)GetResourceBegin(Hello_img),(PtrInt)GetResourceEnd(Hello_img));
	if (0) CreateInnerUserImgProcess((PtrInt)GetResourceBegin(Count1_100_img),(PtrInt)GetResourceEnd(Count1_100_img));
	
	
	if (0)
	{
		for (int i=10000;i<=20000;i+=100)
			(*(char*)i)=i;
	}
	
	if (0)
	{
		A *a=new A(10);
		delete a;
	}
	
	if (0)
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
	VFSM.Init();
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
