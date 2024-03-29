#ifndef POS_PROCESS_HPP
#define POS_PROCESS_HPP

#include "../Types.hpp"
#include "../Error.hpp"
#include "../Trap/Trap.hpp"
#include "../Memory/PhysicalMemory.hpp"
#include "../Memory/VirtualMemory.hpp"
#include "../Library/DataStructure/LinkTable.hpp"
#include "SpinLock.hpp"

const unsigned MaxProcessCount=128;
const Uint64 KernelStackSize=PageSize*4;
const PtrInt InnerUserProcessLoadAddr=0x800020,
			 InnerUserProcessStackSize=PageSize*32,
			 InnerUserProcessStackAddr=0x80000000-InnerUserProcessStackSize;

inline RegisterData GetCPUID()
{
	RegisterData id;
	asm volatile("mv %0,tp":"=r"(id));
	return id;
}

class Process;
class Semaphore;
class FileHandle;

class ProcessManager
{
	friend class Process;
	friend class Semaphore;
	friend void KernelFaultSolver();
	protected:
		static Process Processes[MaxProcessCount];//temp for test...
		static Process *CurrentProcess;
		static Uint32 ProcessCount;
		SpinLock lock;
		
	public:
		Process* GetProcess(PID id);
		Process* AllocProcess();
		ErrorType FreeProcess(Process *proc);
		
		static inline Process* Current()
		{return CurrentProcess;}
		
		static void Schedule();
		
		ErrorType Init();
		ErrorType Destroy();
};
extern ProcessManager POS_PM;

class Process
{
	friend class ProcessManager;
	friend class ForkServerClass;
	friend class Semaphore;
	friend class FileHandle;
	friend void Trap(TrapFrame *tf);
	friend PID Syscall_Clone(TrapFrame *tf,Uint64 flags,void *stack,PID ppid,Uint64 tls,PID cid);//??
	friend void KernelFaultSolver();
	friend int Thread_CreateProcessFromELF(void *userdata);
	friend void TrapFailedInfo(TrapFrame *tf);
	public:
		enum
		{
			S_None=0,
			S_Allocated,
			S_Initing,
			S_Ready,
			S_Running,
			S_UserRunning,
			S_Sleeping,
			S_Quiting
		};
		
		enum:Uint64
		{
			F_Kernel		=1ull<<0,
			F_AutoDestroy	=1ull<<1,
			F_GeneratedStack=1ull<<2,
			F_OutsideName	=1ull<<3
		};
		
		enum
		{
			Exit_Normal=0,
			Exit_Destroy=-10000,
			Exit_BadSyscall,
			Exit_Execve,
			Exit_SegmentationFault
		};
		
		struct RegContext
		{
			RegisterData ra,sp,s[12];
		};
		
		enum:PID
		{
			
			AnyPID    =(PID)-3,
			UnknownPID=(PID)-2,
			InvalidPID=(PID)-1,
		};
		
	protected:
		ProcessManager *PM;//PM fill
		PID ID;//PM fill
		Uint32 stat;
		ClockTime CountingBase,
				  RunningDuration,
				  StartedTime,
				  SleepingDuration,//Sleeping stat
				  WaitingDuration,//Ready stat
				  UserDuration;
		void *Stack;//[Stack,Stack+StackSize) is the stack area
		Uint32 StackSize;
//		void *UserStack;
//		Uint32 UserStackSize;
		Process *fa,
				*pre,
				*nxt,
				*child;
		RegContext context;
//		TrapFrame *tf;
		VirtualMemorySpace *VMS;//if nullptr,means using kernel common area
		Uint64 flags;
		char *Name;
		Uint32 Namespace;//0 means default,unused yet
		int ReturnedValue;
		POS::LinkTable <Process> SemWaitingLink;//Processes waiting in a Semaphore
		ClockTime SemWaitingTargetTime;//Waiting Semaphore timeout+basetime
		Semaphore *WaitSem;//Used for this process to wait for something such as child process
		HeapMemoryRegion *Heap;
		
		char *CurrentWorkDirectory;
		FileHandle* FileTable[8];
		
		ErrorType InitForKernelProcess0();
		ErrorType CopyFileTable(Process *src);
		ErrorType CopyOthers(Process *src);
		ErrorType Start(TrapFrame *tf,bool IsNew);//fork returned by this
		ErrorType InitFileTable();
		ErrorType DestroyFileTable();
		
	public:
		char **CallingStack;//For Debug...
		
		ErrorType Rest();//Hand out CPU and schedule other Process
		ErrorType Run();
		ErrorType Exit(int re);
		ErrorType Start(int (*func)(void*),void *funcdata,PtrInt userStartAddr=0);
		ErrorType SetVMS(VirtualMemorySpace *vms);
		ErrorType SetStack(void *stack,Uint32 size);//if stack is nullptr, means auto create.
//		ErrorType CopyStackContext(Process *src);
		ErrorType SetName(char *name,bool outside);
		ErrorType SetFa(Process *_fa);
		Process* GetQuitingChild(PID cid=AnyPID);//??
		ErrorType SetCWD(const char *path);//Will be dumplicated.(?)
		FileHandle* GetFileHandleFromFD(int fd);
		ErrorType SwitchStat(Uint32 tar);
		
		inline Uint32 GetStat()
		{return stat;}
		
		inline ErrorType SetHeap(HeapMemoryRegion *heap)
		{
			if (Heap!=nullptr)
				return ERR_TargetExist;
			Heap=heap;
			return ERR_None;
		}
		
		inline HeapMemoryRegion* GetHeap()
		{return Heap;}
		
		inline const char* GetCWD()
		{return CurrentWorkDirectory;}
		
		inline Semaphore *GetWaitSem()//Temporaryly use...
		{return WaitSem;}
		
		inline int GetReturnedValue() const
		{return ReturnedValue;}
		
		inline Process* GetFa()
		{return fa;}
		
		inline const char* GetName() const
		{return Name;}
		
		inline VirtualMemorySpace* GetVMS()
		{return VMS;}
		
		inline PID GetPID() const
		{return ID;}
		
		inline bool IsKernelProcess()
		{return flags&F_Kernel;}
		
		inline bool IsUserProcess()
		{return !(flags&F_Kernel);}
		
		inline ClockTime GetRunningDuration(bool update=1)
		{
			if (update)
				SwitchStat(stat);
			return RunningDuration;
		}
		
		inline ClockTime GetStartedTime(bool update=1)
		{
			if (update)
				SwitchStat(stat);
			return StartedTime;
		}
		
		inline ClockTime GetSleepingDuration(bool update=1)
		{
			if (update)
				SwitchStat(stat);
			return SleepingDuration;
		}
		
		inline ClockTime GetWaitingDuration(bool update=1)
		{
			if (update)
				SwitchStat(stat);
			return WaitingDuration;
		}
		
		inline ClockTime GetUserDuration(bool update=1)
		{
			if (update)
				SwitchStat(stat);
			return UserDuration;
		}
		
		ErrorType Init(Uint64 _flags);
		ErrorType Destroy();
};

class ForkServerClass//Outdated...
{
	protected:
		Process *ThisProcess;
		Process *CurrentRequestingProcess;
		TrapFrame *CurrentRequestingProcessTrapFrame;
		SpinLock lock;
		
		static int ForkServerFunc(void *funcdata);
		
	public:
		ErrorType RequestFork(Process *proc,TrapFrame *tf);
		ErrorType Init();
		ErrorType Destroy();
};
extern ForkServerClass ForkServer;

PID CreateKernelThread(int (*func)(void*),void *funcdata=nullptr,Uint64 flags=Process::F_AutoDestroy);
PID CreateKernelProcess(int (*func)(void*),void *funcdata=nullptr,Uint64 flags=Process::F_AutoDestroy);
PID CreateInnerUserImgProcess(PtrInt start,PtrInt end,Uint64 flags=Process::F_AutoDestroy);
#define CreateInnerUserImgProcessWithName(imgName) CreateInnerUserImgProcess((PtrInt)GetResourceBegin(imgName),(PtrInt)GetResourceEnd(imgName))
/*
	F_Kernel is not set in CreateKernelXXX because it will be auto added;
	if F_AutoDestroy is set, it will return UnknownPID(but acctually exist valid PID)
*/

extern "C"
{
	void KernelThreadExit(int re);
	void SwitchToUserStat();
	void SwitchBackKernelStat();
	extern void KernelThreadEntry2();
	extern void UserThreadEntry();
	extern void ProcessSwitchContext(Process::RegContext *from,Process::RegContext *to);
};

#endif
