#ifndef POS_PROCESS_HPP
#define POS_PROCESS_HPP

#include "../Types.hpp"
#include "../Error.hpp"
#include "../Trap/Trap.hpp"
#include "../Memory/PhysicalMemory.hpp"
#include "../Memory/VirtualMemory.hpp"

const unsigned MaxProcessCount=128;
const Uint64 KernelStackSize=PageSize*4;

class Process;

class ProcessManager
{
	friend class Process;
	protected:
		static Process Processes[MaxProcessCount];//temp for test...
		static Process *CurrentProcess;
		static Uint32 ProcessCount;
		
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
	public:
		enum
		{
			S_None=0,
			S_Allocated,
			S_Initing,
			S_Ready,
			S_Running,
			S_Sleeping,
			S_Quiting
		};
		
		enum
		{
			F_Kernel=1<<0,
			F_AutoDestroy=1<<1,
			F_GeneratedStack=1<<2,
		};
		
		enum
		{
			Exit_Normal=0,
			Exit_Destroy=-10000
		};
		
		struct RegContext
		{
			RegisterData ra,sp,s[12];
		};
		
	protected:
		ProcessManager *PM;//PM fill
		PID ID;//PM fill
		Uint32 stat;
		ClockTime CountingBase,
				  RunningTime,
				  StartedTime,
				  SleepingTime,
				  WaitingTime;
		void *Stack;//[Stack,Stack+KernelStackSize) is the stack area
		Uint32 StackSize;
		Process *fa,
				*pre,
				*nxt,
				*child;
		RegContext context;
//		TrapFrame *tf;
		VirtualMemorySpace *VMS;//if nullptr,means using kernel common area
		Uint64 flags;
		char *Name;
		Uint32 Namespace;//0 means default
		Uint32 ReturnedValue;
		
		ErrorType InitForKernelProcess0();
		
	public:
		ErrorType Rest();
		ErrorType Run();
		ErrorType Exit(int re);
		ErrorType Start(int (*func)(void*),void *funcdata);
		ErrorType SetVMS(VirtualMemorySpace *vms);
		ErrorType SetStack(void *stack,Uint32 size);//if stack is nullptr, means auto create.
		ErrorType SetName(char *name);
		
		inline const char* GetName() const
		{return Name;}
		
		inline VirtualMemorySpace* GetVMS()
		{return VMS;}
		
		inline PID GetPID() const
		{return ID;}
		//ClockTime GetXXXTime();
//		ErrorType Fork();//Reserved... It is not usable

		ErrorType Init(Uint64 _flags);
		ErrorType Destroy();
};

extern "C"
{
	void KernelThreadExit(int re);
	extern void KernelThreadEntry();
	extern void ProcessSwitchContext(Process::RegContext *from,Process::RegContext *to);
};

#endif
