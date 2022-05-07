#include <Trap/Clock.h>
#include <Riscv.h>

volatile TickType ClockTick;

void POS_InitClock()
{
	set_csr(sie,MIP_STIP);
	SetNextClockEvent();
	ClockTick=0;
}
