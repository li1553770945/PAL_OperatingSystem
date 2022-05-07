#include <Trap/Trap.hpp>
#include <Riscv.h>
#include <Trap/Clock.h>

extern "C"
{
	void Trap(TrapFrame *tf)
	{
		if ((long long)tf->cause<0)
			if ((tf->cause&0xff)==IRQ_S_TIMER)
			{
				SetNextClockEvent();
				++ClockTick;
				if (ClockTick%100==0)//Test
					SBI_PUTCHAR('*');
			}
	}
	
	extern void __alltraps(void);
}

void POS_InitTrap()
{
	write_csr(sscratch,0);
	write_csr(stvec,&__alltraps);
}