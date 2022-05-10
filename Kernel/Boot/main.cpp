#include <Boot/SystemInfo.h>
#include <Trap/Clock.h>
#include <Trap/Interrupt.h>
#include <Trap/Trap.hpp>
#include <Library/Kout.hpp>
#include <Memory/PhysicalMemory.hpp>
#include <Test.hpp>
extern "C"{ void * __dso_handle = 0 ;}
extern "C"{ void * __cxa_atexit = 0 ;}


int main()
{
	
	using namespace POS;
	PrintSystemInfo();
	POS_InitClock();
	POS_InitTrap();
	InterruptEnable();

	POS_PMM.Init();

	TestPhysicalMemory();

	//Below do nothing...
	auto Sleep=[](int n){while (n-->0);};
	auto DeadLoop=[Sleep](const char *str)
	{
		while (1)
			Sleep(1e8),
			kout<<str;
	};
	// DeadLoop(".");
	return 0;
}
