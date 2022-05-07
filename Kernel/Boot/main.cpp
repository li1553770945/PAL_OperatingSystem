#include <Boot/SystemInfo.h>
#include <Trap/Clock.h>
#include <Trap/Interrupt.h>
#include <Trap/Trap.hpp>
#include <Library/Kout.hpp>

int main()
{
	using namespace POS;
	PrintSystemInfo();
	POS_InitClock();
	POS_InitTrap();
	InterruptEnable();
	
	//Below do nothing...
	auto Sleep=[](int n){while (n-->0);};
	auto DeadLoop=[Sleep](const char *str)
	{
		while (1)
			Sleep(1e8),
			kout<<str;
	};
	DeadLoop(".");
	return 0;
}
