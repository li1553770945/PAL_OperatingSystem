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
#include <HAL/Disk.hpp>
#include <File/FAT32.hpp>
#include <Process/ELF.hpp>
#include <File/FileNodeEX.hpp>
#include <Library/String/String.hpp>
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

//int SDTest(void*)
//{
//	kout[Test]<<"SDCard test ..."<<endl;
//	test_sdcard();
//	kout[Test]<<"SDCard test OK"<<endl;
//	return 0;
//}

char *KernelTextRemember=nullptr;
void CmpKernelText()
{
	MemcpyT<char>(KernelTextRemember,kernelstart,rodataend-kernelstart);
	for (char *i=KernelTextRemember,*j=kernelstart;j<rodataend;++i,++j)
		if (*i!=*j)
			kout[Fault]<<"Kernel text segment differs in "<<i<<endl;
	kout[Test]<<"KernelTextSegment maybe OK"<<endl;
}

int RunAllTestSuits(void*)
{
	auto RunAllFile=[](auto &self,const char *path,int dep=0)->void
	{
		kout<<dep<<": "<<path<<endl;
		constexpr int bufferSize=64;
		char *buffer[bufferSize];
		int skip=0;
		while (1)
		{
//			kout[Debug]<<"A"<<endl; 
			int cnt=VFSM.GetAllFileIn(POS_PM.Current(),path,buffer,bufferSize,skip);
//			kout[Debug]<<"B"<<endl;
//			for (int i=cnt-1;i>=0;--i)
			for (int i=0;i<cnt;++i)
			{
//				kout[Debug]<<"C"<<endl;
				char *child=strSplice(path,"/",buffer[i]);
//				kout[Debug]<<"Child: "<<DataWithSize(buffer[i],strLen(buffer[i])+1)<<endl;
				if (buffer[i][0]!='.')
				{
//					kout[Debug]<<"D"<<endl;
					FileNode *node=VFSM.Open(POS_PM.Current(),child);
//					kout[Debug]<<"E"<<endl;
					if (!node)
						kout[Error]<<"Cannot open file "<<child<<endl;
					else if (node->IsDir())
						self(self,child,dep+1);
					else
					{
//						kout[Debug]<<"F"<<endl;
						kout[Info]<<"Run "<<child<<endl; 
						FileHandle *file=new FileHandle(node);
						PID id=CreateProcessFromELF(file,0,path);
						if (id>0)
						{
//							kout[Debug]<<"G"<<endl;
							Process *proc=POS_PM.Current(),*cp=nullptr;
							while ((cp=proc->GetQuitingChild(id))==nullptr)
								proc->GetWaitSem()->Wait();
							cp->Destroy();
						}
//						kout[Debug]<<"H"<<endl;
						delete file;
					}
					VFSM.Close(node);
//					kout[Debug]<<"I"<<endl;
				}
				Kfree(child);
				Kfree(buffer[i]);
			}
			if (cnt<bufferSize)
				break;
			else skip+=bufferSize;
		}
	};
	
	kout<<"Test all suits..."<<endl;
	POS_PM.Current()->SetCWD("/VFS/FAT32");
	VirtualFileSystem *vfs=new FAT32();
	VFSM.LoadVFS(vfs);

//	kout.SetEnableEffect(0);
//	kout.SwitchTypeOnoff(Info,0);
//	kout.SwitchTypeOnoff(Warning,0);
//	kout.SwitchTypeOnoff(Test,0);
//	kout.SwitchTypeOnoff(Debug,0);
//	kout.SetEnabledType(0);

	RunAllFile(RunAllFile,".");
	
//	kout.SetEnabledType(-1);
//	kout.SwitchTypeOnoff(Debug,1);
//	kout.SwitchTypeOnoff(Test,1);
//	kout.SwitchTypeOnoff(Warning,1);
	
	kout<<"Test all suits OK"<<endl;
	
//	while (1);
	return 0;
}

int RunLibcTest(void*)
{
	auto Run=[](const char *path,int argc=0,char ** const argv=nullptr)
	{
		FileNode *node=VFSM.Open(POS_PM.Current(),path);
		if (!node)
			kout[Error]<<"Cannot open file "<<path<<endl;
		else if (node->IsDir())
			kout[Error]<<"Cannot run directory "<<path<<endl;
		else
		{
			kout[Info]<<"Run "<<path<<endl; 
			FileHandle *file=new FileHandle(node);
			PID id=CreateProcessFromELF(file,0,".",argc,argv);
			if (id>0)
			{
				Process *proc=POS_PM.Current(),*cp=nullptr;
				while ((cp=proc->GetQuitingChild(id))==nullptr)
					proc->GetWaitSem()->Wait();
				cp->Destroy();
			}
			delete file;
		}
		VFSM.Close(node);
	};
	
	auto RunList=[Run](const char *path)
	{
		FileNode *node=VFSM.Open(POS_PM.Current(),path);
		if (!node||node->IsDir())
			kout[Error]<<"RunList: Target "<<path<<" is not shell list"<<endl;
		else
		{
			kout[Info]<<"RunList "<<path<<endl;
			FileHandle *file=new FileHandle(node);
			char *str=new char[file->Size()+1];//Bad in efficiency??
			if (ErrorType err=file->Read(str,file->Size());err<0||err!=file->Size())
				kout[Error]<<"Read string failed!"<<endl;
			str[file->Size()]=0;
			auto [lineCnt,lineStr]=divideStringByChar(str,'\n',1);
			kout[Info]<<"RunList "<<path<<" total "<<lineCnt<<" lines program."<<endl;
			for (int i=0;i<lineCnt;++i)
				if (lineStr[i])
				{
					kout[Info]<<"RunList "<<path<<" line "<<i<<" is "<<lineStr[i]<<endl;
					auto [cnt,ss]=divideStringByChar(lineStr[i],' ',1);
					if (cnt>0
						&&strComp(ss[3],"daemon_failure")
						&&strComp(ss[3],"pthread_exit_cancel")
						&&strComp(ss[3],"rlimit_open_files")
//						&&strComp(ss[3],"dlopen")
						&&strComp(ss[3],"tls_get_new_dtv")
						)
						Run(ss[0],cnt,ss);
					for (int j=0;j<cnt;++j)
						delete ss[j];
					delete ss;
					delete lineStr[i];
				}
				else kout[Info]<<"RunList "<<path<<" line "<<i<<" is empty line."<<endl;
			delete str;
			delete file;
		}
		VFSM.Close(node);
	};
	
	POS_PM.Current()->SetCWD("/VFS/FAT32");
	VirtualFileSystem *vfs=new FAT32();
	VFSM.LoadVFS(vfs);
	
	VFSM.CreateSymbolLink("/lib/ld-musl-riscv64-sf.so.1","/VFS/FAT32/libc.so");
	VFSM.CreateSymbolLink("/VFS/FAT32/dlopen_dso.so","/VFS/FAT32/libdlopen_dso.so");
//	VFSM.CreateSymbolLink("/etc","/VFS/FAT32");
	
	kout.SetEnableEffect(0);
	kout.SwitchTypeOnoff(Info,0);
	kout.SwitchTypeOnoff(Warning,0);
	kout.SwitchTypeOnoff(Test,0);
	kout.SwitchTypeOnoff(Debug,0);
	kout<<"RunLibcTest..."<<endl;
	RunList("run-dynamic.sh");
	RunList("run-static.sh");
	kout<<"RunLibcTest OK"<<endl;
	kout.SetEnableEffect(1);
	kout.SwitchTypeOnoff(Info,1);
	kout.SwitchTypeOnoff(Warning,1);
	kout.SwitchTypeOnoff(Test,1);
	kout.SwitchTypeOnoff(Debug,1);
	
	#ifdef QEMU
	SBI_SHUTDOWN();
	#else
	kout[Fault]<<"No task to run. User control:"<<endl;
	#endif
	return 0;
}

int RunBusyboxTest(void*)
{
	auto Run=[](int argc=0,char ** const argv=nullptr)
	{
		FileNode *node=VFSM.Open(POS_PM.Current(),"busybox");
		if (!node)
			kout[Error]<<"Cannot open file busybox"<<endl;
		else if (node->IsDir())
			kout[Error]<<"Cannot run directory "<<"busybox"<<endl;
		else
		{
			kout[Info]<<"Run busybox "<<argv[0]<<" "<<argv[1]<<endl; 
			FileHandle *file=new FileHandle(node);
			PID id=CreateProcessFromELF(file,0,".",argc,argv);
			if (id>0)
			{
				Process *proc=POS_PM.Current(),*cp=nullptr;
				while ((cp=proc->GetQuitingChild(id))==nullptr)
					proc->GetWaitSem()->Wait();
				cp->Destroy();
			}
			delete file;
		}
		VFSM.Close(node);
	};
	kout[Debug]<<"Run busybox test..."<<endl;
	// kout.SetEnableEffect(0);
	// kout.SwitchTypeOnoff(Info,0);
	// kout.SwitchTypeOnoff(Warning,0);
	// kout.SwitchTypeOnoff(Test,0);
	// kout.SwitchTypeOnoff(Debug,0);

	POS_PM.Current()->SetCWD("/VFS/FAT32");

	char **sh = new char *[2];
	sh[0] =  new char[32];
	sh[1] =  new char[32];
	strCopy(sh[0],"sh");
	strCopy(sh[1],"./busybox_testcode.sh");
	Run(2,sh);
	kout.SetEnableEffect(1);
	kout.SwitchTypeOnoff(Info,1);
	kout.SwitchTypeOnoff(Warning,1);
	kout.SwitchTypeOnoff(Test,1);
	kout.SwitchTypeOnoff(Debug,1);

	kout[Debug]<<"Run bosybox test ok"<<endl;
	delete sh;
	return 0;
}
int RunLmbenchTest(void*)
{
	
	auto Run=[](const char *path,int argc=0,char ** const argv=nullptr)
	{
		FileNode *node=VFSM.Open(POS_PM.Current(),path);
		kout[Debug]<<"runing "<<path<<endl;
		if (!node)
			kout[Error]<<"Cannot open file "<<path<<endl;
		else if (node->IsDir())
			kout[Error]<<"Cannot run directory "<<path<<endl;
		else
		{
			kout[Info]<<"Run "<<path<<endl; 
			FileHandle *file=new FileHandle(node);
			PID id=CreateProcessFromELF(file,0,".",argc,argv);
			if (id>0)
			{
				Process *proc=POS_PM.Current(),*cp=nullptr;
				while ((cp=proc->GetQuitingChild(id))==nullptr)
					proc->GetWaitSem()->Wait();
				cp->Destroy();
			}
			delete file;
		}
		VFSM.Close(node);
	};
	
	auto RunList=[Run](const char *path)
	{
		FileNode *node=VFSM.Open(POS_PM.Current(),path);
		if (!node||node->IsDir())
			kout[Error]<<"RunList: Target "<<path<<" is not shell list"<<endl;
		else
		{
			kout[Info]<<"RunList "<<path<<endl;
			FileHandle *file=new FileHandle(node);
			char *str=new char[file->Size()+1];//Bad in efficiency??
			if (ErrorType err=file->Read(str,file->Size());err<0||err!=file->Size())
				kout[Error]<<"Read string failed!"<<endl;
			str[file->Size()]=0;
			auto [lineCnt,lineStr]=divideStringByChar(str,'\n',1);
			kout[Info]<<"RunList "<<path<<" total "<<lineCnt<<" lines program."<<endl;
			for (int i=0;i<lineCnt;++i)
				if (lineStr[i])
				{
					kout[Info]<<"RunList "<<path<<" line "<<i<<" is "<<lineStr[i]<<endl;
					auto [cnt,ss]=divideStringByChar(lineStr[i],' ',1);
					if (cnt>0)
					{
						if(strComp(ss[0],"echo")==0)
						{
							for(int j=1;j<cnt;j++)
							{
								kout<<ss[j]<<" ";
							}
							kout<<endl;
						}
						else
						{
							Run(ss[0],cnt,ss);
						}
						
					}
						
					for (int j=0;j<cnt;++j)
						delete ss[j];
					delete ss;
					delete lineStr[i];
				}
				else kout[Info]<<"RunList "<<path<<" line "<<i<<" is empty line."<<endl;
			delete str;
			delete file;
		}
		VFSM.Close(node);
	};


	kout[Debug]<<"Run busybox test..."<<endl;
	// kout.SetEnableEffect(0);
	// kout.SwitchTypeOnoff(Info,0);
	// kout.SwitchTypeOnoff(Warning,0);
	// kout.SwitchTypeOnoff(Test,0);
	// kout.SwitchTypeOnoff(Debug,0);

	POS_PM.Current()->SetCWD("/VFS/FAT32");

	RunList("./lmbench_testcode.sh");
	kout.SetEnableEffect(1);
	kout.SwitchTypeOnoff(Info,1);
	kout.SwitchTypeOnoff(Warning,1);
	kout.SwitchTypeOnoff(Test,1);
	kout.SwitchTypeOnoff(Debug,1);

	kout[Debug]<<"Run bosybox test ok"<<endl;
	return 0;
}
int RunLuaTest(void*)
{
	auto Run=[](const char *pat,int argc=0,char ** const argv=nullptr)
	{
		char path[] = "./lua";
		FileNode *node=VFSM.Open(POS_PM.Current(),path);
		kout[Debug]<<"runing "<<path<<" ";
		for(int i=1;i<argc;i++)
		{
			kout[Debug]<<argv[i]<<" ";
		}
		kout[Debug]<<endl;
		if (!node)
			kout[Error]<<"Cannot open file "<<path<<endl;
		else if (node->IsDir())
			kout[Error]<<"Cannot run directory "<<path<<endl;
		else
		{
			FileHandle *file=new FileHandle(node);
			PID id=CreateProcessFromELF(file,0,".",argc,argv);
			if (id>0)
			{
				Process *proc=POS_PM.Current(),*cp=nullptr;
				while ((cp=proc->GetQuitingChild(id))==nullptr)
					proc->GetWaitSem()->Wait();
				if(cp->GetReturnedValue() == 0)
				{
					kout<<"testcase lua "<<argv[1]<<" success"<<endl;
				}
				else
				{	
					kout<<"testcase lua "<<argv[1]<<" fail"<<endl;
				}
				cp->Destroy();
			}
			delete file;
		}
		VFSM.Close(node);
	};
	
	auto RunList=[Run](const char *path)
	{
		FileNode *node=VFSM.Open(POS_PM.Current(),path);
		if (!node||node->IsDir())
			kout[Error]<<"RunList: Target "<<path<<" is not shell list"<<endl;
		else
		{
			kout[Info]<<"RunList "<<path<<endl;
			FileHandle *file=new FileHandle(node);
			char *str=new char[file->Size()+1];//Bad in efficiency??
			if (ErrorType err=file->Read(str,file->Size());err<0||err!=file->Size())
				kout[Error]<<"Read string failed!"<<endl;
			str[file->Size()]=0;
			auto [lineCnt,lineStr]=divideStringByChar(str,'\n',1);
			kout[Info]<<"RunList "<<path<<" total "<<lineCnt<<" lines program."<<endl;
			for (int i=0;i<lineCnt;++i)
				if (lineStr[i])
				{
					kout[Info]<<"RunList "<<path<<" line "<<i<<" is "<<lineStr[i]<<endl;
					auto [cnt,ss]=divideStringByChar(lineStr[i],' ',1);
					if (cnt>0)
					{
						if(strComp(ss[0],"echo")==0)
						{
							for(int j=1;j<cnt;j++)
							{
								kout<<ss[j]<<" ";
							}
							kout<<endl;
						}
						else
						{
							Run(ss[0],cnt,ss);
						}
						
					}
						
					for (int j=0;j<cnt;++j)
						delete ss[j];
					delete ss;
					delete lineStr[i];
				}
				else kout[Info]<<"RunList "<<path<<" line "<<i<<" is empty line."<<endl;
			delete str;
			delete file;
		}
		VFSM.Close(node);
	};


	kout[Debug]<<"run lua test..."<<endl;
	kout.SetEnableEffect(0);
	kout.SwitchTypeOnoff(Info,0);
	kout.SwitchTypeOnoff(Warning,0);
	kout.SwitchTypeOnoff(Test,0);
	kout.SwitchTypeOnoff(Debug,0);

	POS_PM.Current()->SetCWD("/VFS/FAT32");

	RunList("./lua_testcode.sh");
	kout.SetEnableEffect(1);
	kout.SwitchTypeOnoff(Info,1);
	kout.SwitchTypeOnoff(Warning,1);
	kout.SwitchTypeOnoff(Test,1);
	kout.SwitchTypeOnoff(Debug,1);

	kout[Debug]<<"run lua test ok"<<endl;
	return 0;
}
void TestFuncs()
{
//	kout.SwitchTypeOnoff(Test,0);
	
	if (1)
	{
		kout[Test]<<"PhysicalMemorySize:          "<<(void*)PhysicalMemorySize()<<endl;
		kout[Test]<<"PhysicalMemoryPhysicalStart: "<<(void*)PhysicalMemoryPhysicalStart()<<endl;
		kout[Test]<<"PhymemVirmemOffset:          "<<(void*)PhymemVirmemOffset()<<endl;
		kout[Test]<<"PhysicalMemoryVirtualEnd:    "<<(void*)PhysicalMemoryVirtualEnd()<<endl;
		kout[Test]<<"FreeMemBase:                 "<<(void*)FreeMemBase()<<endl;
		kout[Test]<<"kernelstart:                 "<<(void*)kernelstart<<endl;
		kout[Test]<<"textstart:                   "<<(void*)textstart<<endl;
		kout[Test]<<"textend:                     "<<(void*)textend<<endl;
		kout[Test]<<"rodatastart:                 "<<(void*)rodatastart<<endl;
		kout[Test]<<"rodataend:                   "<<(void*)rodataend<<endl;
		kout[Test]<<"datastart:                   "<<(void*)datastart<<endl;
		kout[Test]<<"dataend:                     "<<(void*)dataend<<endl;
		kout[Test]<<"bssstart:                    "<<(void*)bssstart<<endl;
		kout[Test]<<"bssend:                      "<<(void*)bssend<<endl;
		kout[Test]<<"kernelend:                   "<<(void*)kernelend<<endl;
		kout[Test]<<"bootstack:                   "<<(void*)bootstack<<endl;
		kout[Test]<<"bootstacktop:                "<<(void*)bootstacktop<<endl;
	}
	
	VirtualMemorySpace::EnableAccessUser();
	
	if (0)
	{
		KernelTextRemember=(char*)Kmalloc(rodataend-kernelstart);
		MemcpyT<char>(KernelTextRemember,kernelstart,rodataend-kernelstart);
		CmpKernelText();
	}
	
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
	if (1) CreateInnerUserImgProcessWithName(ForkTest_img);
	
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
		CreateKernelThread([](void*)->int
		{
			kout<<"MainTest: Test sleep using Semaphore... Time: "<<GetClockTime()/Timer_1ms<<"ms"<<endl;
			Semaphore sem(0);
			sem.Wait(Timer_1s*10);
			kout<<"MainTest: Test sleep using Semaphore OK. Time: "<<GetClockTime()/Timer_1ms<<"ms. Semaphore value: "<<sem.Value()<<endl;
		},nullptr);
	
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
		int skip=0;
		ISAS
		while (1)
		{
			int cnt=vfs->GetAllFileIn(path,buffer,16,skip);
			for (int i=0;i<cnt;++i)
			{
				char *child=strSplice(path,dep==0?"":"/",buffer[i]);
				if (buffer[i][0]!='.')//??
					self(self,vfs,child,dep+1);
				Kfree(child);
				Kfree(buffer[i]);
			}
			if (cnt<16)
				break;
			else skip+=16;
		}
	};
	
	auto PrintVFSM=[](auto &self,const char *path,int dep=0)->void
	{
		kout<<dep<<": "<<path<<endl;
		char *buffer[16];
		int skip=0;
		ISAS
		while (1)
		{
			int cnt=VFSM.GetAllFileIn(path,buffer,16,skip);
			for (int i=0;i<cnt;++i)
			{
				char *child=strSplice(path,dep==0?"":"/",buffer[i]);
				if (buffer[i][0]!='.')//??
					self(self,child,dep+1);
				Kfree(child);
				Kfree(buffer[i]);
			}
			if (cnt<16)
				break;
			else skip+=16;
		}
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
	
//	if (0) CreateKernelThread(SDTest,nullptr);
	
//	if (0)
//	{
//		Sector sec;
//		sdcard_read_sector(&sec,0);
//		kout[Debug]<<DataWithSizeUnited(&sec,sizeof(Sector),32)<<endl;
//		sdcard_read_sector(&sec,32);
//		kout[Debug]<<DataWithSizeUnited(&sec,sizeof(Sector),32)<<endl;
//		sdcard_read_sector(&sec,8098);
//		kout[Debug]<<DataWithSizeUnited(&sec,sizeof(Sector),32)<<endl;
//	}
		
	if (0)
	{
		kout<<"FAT32 Test:"<<endl;
		VirtualFileSystem *vfs=new FAT32();
		Printfs(Printfs,vfs,"/");
		delete vfs;
		kout<<"FAT32 Test OK"<<endl;
	}
	
	if (0)
	{
		kout<<"VFSM Test:"<<endl;
		VirtualFileSystem *vfs=new FAT32();
		VFSM.LoadVFS(vfs);
		VFSM.CreateDirectory("/VFS/FAT32/MyDir");
		PrintVFSM(PrintVFSM,"/");
//		delete vfs;
		kout<<"VFSM Test OK"<<endl;
	}
	
	if (0)
	{
//		kout<<"Test ELF ..."<<endl;
//		VirtualFileSystem *vfs=new FAT32();
//		FileNode *node=vfs->Open("/dir/TEST.elf");
//		if (!node)
//			kout[Fault]<<"Cannot open /dir/TEST.elf"<<endl;
//		FileHandle *file=new FileHandle(node);
//		CreateProcessFromELF(file,Process::F_AutoDestroy);
//		delete file;
//		delete node;
//		delete vfs;
//		kout<<"Test ELF OK"<<endl;
	}
	
	if (0)
	{
		PID id=CreateKernelThread(RunAllTestSuits,nullptr,0);
		if (0)
		{
			Process *proc=POS_PM.Current(),*cp;
			while ((cp=proc->GetQuitingChild(id))==nullptr)
				proc->GetWaitSem()->Wait();
			cp->Destroy();
		}
	}
	
	VirtualFileSystem *vfs=new FAT32();
	VFSM.LoadVFS(vfs);
	if (0)
	{

		PID id=CreateKernelThread(RunLuaTest,nullptr,0);
		if (1)
		{
			Process *proc=POS_PM.Current(),*cp;
			while ((cp=proc->GetQuitingChild(id))==nullptr)
				proc->GetWaitSem()->Wait();
			cp->Destroy();
		}
	}
	if (0)
	{
		PID id=CreateKernelThread(RunBusyboxTest,nullptr,0);
		if (1)
		{
			Process *proc=POS_PM.Current(),*cp;
			while ((cp=proc->GetQuitingChild(id))==nullptr)
				proc->GetWaitSem()->Wait();
			cp->Destroy();
		}
	}
	if (0)
	{
		PID id=CreateKernelThread(RunLmbenchTest,nullptr,0);
		if (1)
		{
			Process *proc=POS_PM.Current(),*cp;
			while ((cp=proc->GetQuitingChild(id))==nullptr)
				proc->GetWaitSem()->Wait();
			cp->Destroy();
		}
	}
	kout<<"!TEST FINISH!"<<endl;
}

void InitBSS()
{
	for (char *i=bssstart;i<bssend;++i)
		if (!InRange(i,bootstack,bootstacktop-1))
			*i=0;
}

int main()
{
	PrintSystemInfo();
	
	set_csr(sstatus,0x2000);
	
	POS_InitClock();
	POS_InitTrap();
	InitBSS();
	POS_PMM.Init();
	VirtualMemorySpace::InitStatic();
	POS_PM.Init();
//	PrintDeviceTree((void*)DTB+PhymemVirmemOffset()+PhysicalMemoryPhysicalStart());
	DiskInit();
	VFSM.Init();
	InterruptEnable();
	
	TestFuncs();
	
	//Below do nothing...
	
	Process *cur=POS_PM.Current();
	while (1)
		cur->Rest();
	
	auto Sleep=[](int n){while (n-->0);};
	auto DeadLoop=[Sleep](const char *str)
	{
		while (1)
		{
			Sleep(1e8),
			kout<<DarkGray<<str<<Reset;
		}
	};
	DeadLoop(".");
	return 0;
}
