#ifndef POS_TRAP_HPP
#define POS_TRAP_HPP

#include "../Types.hpp"

struct RegisterContext
{
	union
	{
		RegisterData x[32];
		struct
		{
			RegisterData zero,ra,sp,gp,tp,
						 t0,t1,t2,
						 s0,s1,
						 a0,a1,a2,a3,a4,a5,a6,a7,
						 s2,s3,s4,s5,s6,s7,s8,s9,s10,s11,
						 t3,t4,t5,t6;
		};
	};
	
	inline RegisterData& operator [] (int index)
	{return x[index];}
};

struct TrapFrame
{
	RegisterContext reg;
	RegisterData status,
				 epc,
				 badvaddr,
				 cause;
};

enum
{
	ExceptionCode_InstructionAddressMisaligned	=0,
	ExceptionCode_InstructionAccessFault		=1,
	ExceptionCode_IllegalInstruction			=2,
	ExceptionCode_BreakPoint					=3,
	ExceptionCode_LoadAddressMisaligned			=4,
	ExceptionCode_LoadAccessFault				=5,
	ExceptionCode_StoreAddressMisaligned		=6,
	ExceptionCode_StoreAccessFault				=7,
	ExceptionCode_UserEcall						=8,
	ExceptionCode_SupervisorEcall				=9,
	ExceptionCode_HypervisorEcall				=10,
	ExceptionCode_MachineEcall					=11,
	ExceptionCode_InstructionPageFault			=12,
	ExceptionCode_LoadPageFault					=13,
	ExceptionCode_StorePageFault				=15
};

enum
{
	InterruptCode_SupervisorSoftwareInterrupt	=1,
	InterruptCode_MachineSoftwareInterrupt		=3,
	InterruptCode_SupervisorTimerInterrupt		=5,
	InterruptCode_MachineTimerInterrupt			=7,
	InterruptCode_SupervisorExternalInterrupt	=9,
	InterruptCode_MachineExternalInterrupt		=11
};

extern "C" void Trap(TrapFrame *tf);

void POS_InitTrap();

#endif
