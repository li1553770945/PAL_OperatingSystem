#ifndef POS_SYNCHRONIZE_HPP
#define POS_SYNCHRONIZE_HPP

#include "Process.hpp"
#include "../Trap/Interrupt.hpp"
#include "../Trap/Clock.h"
#include "../SyscallID.hpp"

extern bool OnTrap;
class Semaphore
{
	public:
		enum:Uint64
		{
			TryWait=0,
			KeepWait=(Uint64)-1
		};
		enum:unsigned
		{
			SignalAll=0,
			SignalOne=1
		};
		
	protected:
		POS::LinkTable <Process> Head,Tail;
		int value=0;
		
		inline void LockProcess()
		{POS_PM.lock.Lock();}
		
		inline void UnlockProcess()
		{POS_PM.lock.Unlock();}
		
	public:
		inline bool Wait(Uint64 timeOut=KeepWait)
		{
			InterruptStackSaver iss;
			iss.Save();
			LockProcess();
			--value;
			bool re=1;
			if (value<0)
				if (timeOut==TryWait)
					re=0,++value;
				else
				{
					Process *proc=POS_PM.Current();
					Tail.PreInsert(&proc->SemWaitingLink);
					if (timeOut!=KeepWait)
						proc->SemWaitingTargetTime=GetClockTime()+timeOut;
					proc->SwitchStat(Process::S_Sleeping);
					
					UnlockProcess();
					iss.Restore();
					
//					POS_PM.Schedule();
					
					if (OnTrap)
						ProcessManager::Schedule();
					else
					{
						RegisterData a7=SYS_Rest;
						asm volatile("ld a7,%0; ebreak"::"m"(a7):"memory");
					}
					
					iss.Save();
					LockProcess();
					if (timeOut!=KeepWait&&GetClockTime()>=proc->SemWaitingTargetTime)
						re=0,++value;//??
					proc->SemWaitingLink.Remove();//?? It may reach here because of timeout etc...
					proc->SemWaitingTargetTime=0;
				}
			UnlockProcess();
			iss.Restore();
			return re;
		}
		
		inline void Signal(unsigned n=SignalOne)
		{
			ISASBC;
			LockProcess();
			if (n==SignalAll)
				n=POS::maxN(-value,0);
			value+=n;
			while ((Sint64)n-->0)
			{
				POS::LinkTable <Process> &p=*Head.Nxt();
				if (!p.NxtEmpty())
				{
					if (p()->stat==Process::S_Sleeping)
						p()->SwitchStat(Process::S_Ready);
				}
				else break;
			}
			UnlockProcess();
		}
		
		inline int Value()
		{
			ISASBC;
			LockProcess();
			int re=value;
			UnlockProcess();
			return re;
		}
		
		~Semaphore()
		{
			Signal(SignalAll);
		}
		
		Semaphore(int initValue):value(initValue)
		{
			Head.NxtInsert(&Tail);
		}
		
		Semaphore(const Semaphore&)=delete;
		Semaphore(const Semaphore&&)=delete;
		Semaphore& operator = (const Semaphore&)=delete;
		Semaphore& operator = (const Semaphore&&)=delete;
}; 

class Mutex:public Semaphore
{
	public:
		inline void Lock()
		{Wait();}
		
		inline void Unlock()
		{Signal();}
		
		inline bool TryLock()
		{return Wait(TryWait);}
		
		Mutex():Semaphore(1) {}
};

#endif
