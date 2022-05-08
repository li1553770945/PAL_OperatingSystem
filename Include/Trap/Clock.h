#ifndef POS_CLOCK_H
#define POS_CLOCK_H

#include "../Types.h"
#include "../SBI.h"

const ClockTime Timer_1ms=1e4;
const ClockTime Timer_10ms=Timer_1ms*10;
const ClockTime Timer_100ms=Timer_1ms*100;
const ClockTime Timer_1s=Timer_1ms*1000;
const ClockTime TickDuration=Timer_1ms;//??

extern volatile TickType ClockTick;

static inline void SetClockTimeEvent(ClockTime t)
{
	SBI_SET_TIMER(t);
}

static inline ClockTime GetClockTime()
{
	ClockTime t;
    asm volatile("rdtime %0" : "=r"(t));
    return t;
}

static inline void SetNextClockEvent()
{
	SetClockTimeEvent(GetClockTime()+TickDuration);
}

void POS_InitClock();
#endif
