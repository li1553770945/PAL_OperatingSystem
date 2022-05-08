#include <Trap/Trap.hpp>
#include <Riscv.h>
#include <Trap/Clock.h>
#include <Library/Kout.hpp>
using namespace POS;
extern "C"
{
	void Trap(TrapFrame *tf)
	{
		if ((long long)tf->cause<0)
		{
			int cause = (tf->cause << 1) >> 1; 
			switch (cause) 
			{
				case IRQ_U_SOFT:
					Puts("User software interrupt\n");
					break;
				case IRQ_S_SOFT:
					Puts("Supervisor software interrupt\n");
					break;
				case IRQ_H_SOFT:
					Puts("Hypervisor software interrupt\n");
					break;
				case IRQ_M_SOFT:
					Puts("Machine software interrupt\n");
					break;
				case IRQ_U_TIMER:
					Puts("User software interrupt\n");
					break;
				case IRQ_S_TIMER:
					//时钟中断
					SetNextClockEvent();
						++ClockTick;
						if (ClockTick%100==0)//Test
							Puts("Timer interrupt\n");
					break;
				case IRQ_H_TIMER:
					Puts("Hypervisor software interrupt\n");
					break;
				case IRQ_M_TIMER:
					Puts("Machine software interrupt\n");
					break;
				case IRQ_U_EXT:
					Puts("User software interrupt\n");
					break;
				case IRQ_S_EXT:
					Puts("Supervisor external interrupt\n");
					break;
				case IRQ_H_EXT:
					Puts("Hypervisor software interrupt\n");
					break;
				case IRQ_M_EXT:
					Puts("Machine software interrupt\n");
					break;
				default:
					Puts("unknown interrupt");
					break;
			}
    	}
		else
		{
			switch (tf->cause) 
			{
				case CAUSE_MISALIGNED_FETCH:
					break;
				case CAUSE_FAULT_FETCH:
					break;
				case CAUSE_ILLEGAL_INSTRUCTION:
					break;
				case CAUSE_BREAKPOINT:
					break;
				case CAUSE_MISALIGNED_LOAD:
					break;
				case CAUSE_FAULT_LOAD:
					break;
				case CAUSE_MISALIGNED_STORE:
					break;
				case CAUSE_FAULT_STORE:
					break;
				case CAUSE_USER_ECALL:
					break;
				case CAUSE_SUPERVISOR_ECALL:
					break;
				case CAUSE_HYPERVISOR_ECALL:
					break;
				case CAUSE_MACHINE_ECALL:
					break;
				default:
					// Puts("unknown exception");
					break;
			}
		}
	}
	extern void __alltraps(void);
}

void POS_InitTrap()
{
	write_csr(sscratch,0);
	write_csr(stvec,&__alltraps);
}