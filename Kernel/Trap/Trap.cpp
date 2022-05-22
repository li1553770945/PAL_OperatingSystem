#include <Trap/Trap.hpp>
#include <Riscv.h>
#include <Trap/Clock.h>
#include <Library/Kout.hpp>
#include <Error.hpp>
#include <Process/Process.hpp>
#include <Memory/VirtualMemory.hpp>
#include <Trap/Syscall.hpp>

extern "C"
{
	void Trap(TrapFrame *tf)
	{
		using namespace POS;
		ErrorType err=0;
		if ((long long)tf->cause<0) switch(tf->cause<<1>>1) 
		{
			case InterruptCode_SupervisorTimerInterrupt:
				SetNextClockEvent();
				++ClockTick;
				if (ClockTick%100==0)//Test
					kout<<LightGray<<"*"<<Reset;
				if (ClockTick%1000==0)
					POS_PM.Schedule();
				break;
			default:
				kout[Warning]<<"Unknown interrupt:"<<endline
							 <<"  casue   :"<<(void*)tf->cause<<endline
							 <<"  badvaddr:"<<(void*)tf->badvaddr<<endline
							 <<"  epc     :"<<(void*)tf->epc<<endline
							 <<"  status  :"<<(void*)tf->status<<endl;
		}
		else switch (tf->cause)
		{
			case ExceptionCode_BreakPoint://??
			case ExceptionCode_UserEcall:
				err=TrapFunc_Syscall(tf);
				break;
			case ExceptionCode_InstructionPageFault:
			case ExceptionCode_LoadPageFault:
			case ExceptionCode_StorePageFault:
				err=TrapFunc_FageFault(tf);
				break;
			default:
				kout[Fault]<<"Unknown exception:"<<endline
						   <<"  casue   :"<<(void*)tf->cause<<endline
						   <<"  badvaddr:"<<(void*)tf->badvaddr<<endline
						   <<"  epc     :"<<(void*)tf->epc<<endline
						   <<"  status  :"<<(void*)tf->status<<endl;
		}
		if (err!=0)
		{
			kout[Fault]<<"TrapFunc failed:"<<endline
					   <<"  cause   :"<<(void*)tf->cause<<endline
					   <<"  vaddr   :"<<(void*)tf->badvaddr<<endline
				       <<"  epc     :"<<(void*)tf->epc<<endline
				       <<"  status  :"<<(void*)tf->status<<endline
				       <<" TrapFunc ErrorType:"<<err<<endl;
		}
	}
	
	extern void __alltraps();
}

void POS_InitTrap()
{
	write_csr(sscratch,0);
	write_csr(stvec,&__alltraps);
}
