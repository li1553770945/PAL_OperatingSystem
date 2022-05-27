#include <Boot/SystemInfo.h>
#include <Trap/Clock.h>
#include <Trap/Interrupt.hpp>
#include <Trap/Trap.hpp>
#include <Library/Kout.hpp>
#include <Process/Process.hpp>
#include <Memory/VirtualMemory.hpp>
#include <Resources.hpp>
#include <File/FileSystem.hpp>
#include <File/StupidFileSystem.hpp>
#include <Library/String/SysStringTools.hpp>
#include <Process/Synchronize.hpp>
#include <Riscv.h>
extern "C"
{
	#include <HAL/Drivers/_plic.h>
	#include <HAL/Drivers/_fpioa.h>
	#include <HAL/Drivers/_dmac.h>
};
#include <HAL/Drivers/_sdcard.h>
#include <File/FAT32.hpp>
//#include <HAL/DeviceTreeBlob.hpp>

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

int SemaphoreTest(void *data)
{
	Semaphore *sem=(Semaphore*)data;
	kout<<"SemaphoreTest: Do something..."<<endl;
	for (int i=0;i<=5;++i)
		for (int j=0;j<=1e8;++j)
			;
	kout<<"SemaphoreTest: Do something OK, Signal Semaphore"<<endl;
	sem->Signal();
	kout<<"SemaphoreTest: Do something OK, Signal Semaphore OK"<<endl;
	return 0;
}

int SemaphoreTest2(void *data)
{
	Semaphore *sem=(Semaphore*)data;
	kout<<"SemaphoreTest2: Wait Semaphore..."<<endl;
	sem->Wait();
	kout<<"SemaphoreTest2: Wait Semaphore OK, Signal again"<<endl;
	sem->Signal();
	kout<<"SemaphoreTest2: Signal OK"<<endl;
	return 0;
}

int SDTest(void*)
{
	kout[Test]<<"SDCard test ..."<<endl;
	test_sdcard();
	kout[Test]<<"SDCard test OK"<<endl;
	return 0;
}

void TestFuncs()
{
//	kout.SwitchTypeOnoff(Test,0);
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
	
	if (0)
	{
		kout[Test]<<"Test PageFault"<<endl;
		for (int i=10000;i<=20000;i+=100)
			(*(char*)i)=i;
		kout[Test]<<"Test PageFault OK"<<endl;
	}
	
	if (0) CreateKernelThread(KernelThreadTest,nullptr);
	if (0) CreateKernelThread(KernelThreadTest2,nullptr);
	if (0) CreateInnerUserImgProcessWithName(Hello_img);
	if (0) CreateInnerUserImgProcessWithName(Count1_100_img);
	if (0) CreateInnerUserImgProcessWithName(ForkTest_img);
	
	if (0)
	{
		Semaphore sem(0);
		CreateKernelThread(SemaphoreTest,&sem);
		CreateKernelThread(SemaphoreTest2,&sem);
		for (int i=0;i<=10;++i)
			for (int j=0;j<=1e8;++j)
				;
		kout<<"MainTest: Wait Semaphore..."<<endl;
		sem.Wait();
		kout<<"MainTest: Wait Semaphore OK"<<endl;
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
	
	auto Printfs=[](auto &self,auto *vfs,const char *path,int dep=0)->void
	{
		kout<<dep<<": "<<path<<endl;
		char *buffer[16];
		int cnt=vfs->GetAllFileIn(path,buffer,16);
		for (int i=0;i<cnt;++i)
		{
			char *child=strSplice(path,"/",buffer[i]);
			self(self,vfs,child,dep+1);
			Kfree(child);
			Kfree(buffer[i]);
		}
		if (cnt==16)
			kout<<dep<<": ..."<<endl;
	};
	
	if (0)
	{
		VirtualFileSystem *vfs=new StupidFileSystem();
		vfs->CreateDirectory("/Dir1");
		vfs->CreateFile("/Dir1/File1");
		vfs->CreateFile("/Dir1/File2");
		vfs->CreateDirectory("/Dir2");
		vfs->CreateDirectory("/Dir3");
		vfs->CreateDirectory("/Dir3/Dir4");
		vfs->CreateFile("/Dir3/Dir4/File3");
		kout<<"StupidFileSystem Test:"<<endl;
		Printfs(Printfs,vfs,"/");
		delete vfs;
	}
	
	if (0) CreateKernelThread(SDTest,nullptr);
	
	if (0)
	{
		Sector sec;
		sdcard_read_sector(&sec,0);
		kout[Debug]<<DataWithSizeUnited(&sec,sizeof(Sector),32)<<endl;
		sdcard_read_sector(&sec,32);
		kout[Debug]<<DataWithSizeUnited(&sec,sizeof(Sector),32)<<endl;
		sdcard_read_sector(&sec,8098);
		kout[Debug]<<DataWithSizeUnited(&sec,sizeof(Sector),32)<<endl;
	}
		
	if (0)
	{
		VirtualFileSystem *vfs=new FAT32();
		kout<<"FAT32 Test:"<<endl;
		FileNode * node1 =vfs->Open("/LS");
		FileNode * node2 =vfs->Open("/pos/main.cpp");
		kout[Debug]<<node1<<" "<<node2<<endl;
		char *buffer=new char[10000];
		MemsetT<char>(buffer,0,sizeof(buffer));
		kout[Debug]<<node2->Read(buffer,0,node2->Size())<<endl;
		kout[Debug]<<buffer<<endl;
		delete[] buffer;
//		Printfs(Printfs,vfs,"/");
		kout<<"FAT32 Test OK"<<endl;
		delete vfs;
	}
}

int main(RegisterData hartID,RegisterData DTB)
{
	PrintSystemInfo();
	POS_InitClock();
	POS_InitTrap();
	POS_PMM.Init();
	kout<<"A"<<endl;
	VirtualMemorySpace::InitStatic();
	kout<<"B"<<endl;
	POS_PM.Init();
	kout<<"C"<<endl;
	ForkServer.Init();
//	PrintDeviceTree((void*)DTB+PhymemVirmemOffset()+PhysicalMemoryPhysicalStart());
	
	kout[Info]<<"plic init..."<<endl;
	plicinit();
	kout[Info]<<"plic init hart..."<<endl;
    plicinithart();
	kout[Info]<<"fpioa init..."<<endl;
    fpioa_pin_init();
	kout[Info]<<"dmac init..."<<endl;
    dmac_init();
	kout[Info]<<"SDCard init..."<<endl;
	sdcard_init();
	kout[Info]<<"Drivers init OK"<<endl;
	
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
