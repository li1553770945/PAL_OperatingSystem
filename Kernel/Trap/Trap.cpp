#include <Trap/Trap.hpp>
#include <Riscv.h>
#include <Trap/Clock.h>
#include <Library/Kout.hpp>
#include <Error.hpp>
#include <Process/Process.hpp>
#include <Memory/VirtualMemory.hpp>
#include <Trap/Syscall.hpp>
#include <HAL/Disk.hpp>
#include <Trap/Interrupt.hpp>
extern "C"
{
	#include <HAL/Drivers/_plic.h>
};

static const char* TrapInterruptCodeName[16]=
{
	"InterruptCode_0"							,
	"InterruptCode_SupervisorSoftwareInterrupt" ,
	"InterruptCode_2"							,
	"InterruptCode_MachineSoftwareInterrupt"	,
	"InterruptCode_4"							,
	"InterruptCode_SupervisorTimerInterrupt"	,
	"InterruptCode_6"							,
	"InterruptCode_MachineTimerInterrupt"		,
	"InterruptCode_8"							,
	"InterruptCode_SupervisorExternalInterrupt"	,
	"InterruptCode_10"							,
	"InterruptCode_MachineExternalInterrupt"	,
	"InterruptCode_12"							,
	"InterruptCode_13"							,
	"InterruptCode_14"							,
	"InterruptCode_15"
};

static const char* TrapExceptionCodeName[16]=
{
	"ExceptionCode_InstructionAddressMisaligned" ,
	"ExceptionCode_InstructionAccessFault"       ,
	"ExceptionCode_IllegalInstruction"           ,
	"ExceptionCode_BreakPoint"                   ,
	"ExceptionCode_LoadAddressMisaligned"        ,
	"ExceptionCode_LoadAccessFault"              ,
	"ExceptionCode_StoreAddressMisaligned"       ,
	"ExceptionCode_StoreAccessFault"             ,
	"ExceptionCode_UserEcall"	                 ,
	"ExceptionCode_SupervisorEcall"              ,
	"ExceptionCode_HypervisorEcall"              ,
	"ExceptionCode_MachineEcall"                 ,
	"ExceptionCode_InstructionPageFault"         ,
	"ExceptionCode_LoadPageFault"                ,
	"ExceptionCode_14"                           ,
	"ExceptionCode_StorePageFault"
};

bool OnTrap=0;//For debug

void TrapFailedInfo(TrapFrame *tf)
{
	using namespace POS;
	Process *cur=POS_PM.Current();
	if ((tf->cause<<1>>1)<16)
		kout<<"  TrapType:"<<((long long)tf->cause<0?TrapInterruptCodeName:TrapExceptionCodeName)[tf->cause&0xF]<<endline;
	kout	<<"  cause   :"<<(void*)tf->cause<<endline
			<<"  vaddr   :"<<(void*)tf->badvaddr<<endline
			<<"  epc     :"<<(void*)tf->epc<<endline
			<<"  status  :"<<(void*)tf->status<<endline
			<<"  ra      :"<<(void*)tf->reg.ra<<endline
			<<"  PID     :"<<cur->GetPID()<<endl;
}

#define TrapFailed(msg) (kout[Fault]<<msg<<endline,TrapFailedInfo(tf))

extern "C"
{
	void Trap(TrapFrame *tf)
	{
		using namespace POS;
//		kout[Test]<<"<<"<<(void*)tf->epc<<endl;
		Process *cur=POS_PM.Current();
		bool FromUser=cur->IsUserProcess()&&cur->GetStat()==Process::S_UserRunning;
		if (FromUser)
			SwitchBackKernelStat();
		if (OnTrap)
			;//TrapFailed("Trap OnTrap");
		else OnTrap=1;
//		if (cur->OnTrap)
//			;//TrapFailed("Trap ProcessOnTrap");
//		else cur->OnTrap=1;
//		InterruptEnable();
		if ((long long)tf->cause<0) switch(tf->cause<<1>>1) 
		{
//			case InterruptCode_SupervisorSoftwareInterrupt:
//				break;
			case InterruptCode_SupervisorTimerInterrupt:
				SetNextClockEvent();
				++ClockTick;
				if (ClockTick%30==0)//300 is just for test...
					POS_PM.Schedule();
				break;
			case InterruptCode_SupervisorExternalInterrupt://Need improve...
			{
				int irq=plic_claim();
//				kout[Debug]<<"irq "<<irq<<endl;
				switch (irq)
				{
					case 0:	break;
					case UART_IRQ:
						//...
						break;
					case DISK_IRQ:
					{
						ErrorType err=DiskInterruptSolve();
						if (err)
							TrapFailed("DiskInterruptSolve failed! ErrorCode "<<err);
						break;
					}
					default:
						TrapFailed("Unknown platform level interrupt "<<irq<<" !");
				}
				if (irq)
					plic_complete(irq);
//				#ifndef QEMU//??
//				w_sip(r_sip()&~2);    // clear pending bit
//				sbi_set_mie();
//				#endif
				break;
			}
			default:
				TrapFailed("Unknown interrupt!");
		}
		else switch (tf->cause)
		{
			case ExceptionCode_BreakPoint://??
			case ExceptionCode_UserEcall:
			{
				ErrorType err=TrapFunc_Syscall(tf);
				if (err)
					TrapFailed("TrapFunc_Syscall failed! ErrorCode"<<err);
				else tf->epc+=tf->cause==ExceptionCode_UserEcall?4:2;
				break;
			}
			case ExceptionCode_LoadAccessFault:
			case ExceptionCode_StoreAccessFault:
			case ExceptionCode_InstructionPageFault:
			case ExceptionCode_LoadPageFault:
			case ExceptionCode_StorePageFault:
			{
				ErrorType err=TrapFunc_FageFault(tf);
				if (InThisSet(err,ERR_OutOfMemory,ERR_AccessInvalidVMR))//Debug...
				{
					if (tf->status&0x100)
						kout[Fault]<<"TrapFunc_FageFault failed in kernel! ErrorType "<<err<<endline;
					else kout[Error]<<"TrapFunc_FageFault failed! Exit this process! ErrorType "<<err<<endline;
					TrapFailedInfo(tf);
					cur->Exit(Process::Exit_SegmentationFault);
					POS_PM.Schedule();
					kout[Fault]<<"Unreachable area!"<<endl;
				}
				if (err)
					TrapFailed("TrapFunc_PageFault failed! ErrorCode "<<err);
				break;
			}
			default:
				TrapFailed("Unsolveable exception!");
		}
		if (FromUser)
			SwitchToUserStat();
//		InterruptDisable();
		if (!OnTrap)
			;//TrapFailed("~Trap NotOnTrap");
		else OnTrap=0;
//		if (!cur->OnTrap)
//			;//TrapFailed("~Trap ProcessNotOnTrap");
//		else cur->OnTrap=0;
//		kout[Test]<<(void*)tf->epc<<">>"<<endl;
	}
	
	extern void __alltraps();
}

void POS_InitTrap()
{
	set_csr(sie,MIP_SSIP);
	set_csr(sie,MIP_SEIP);
	write_csr(sscratch,0);
	write_csr(stvec,&__alltraps);
}
