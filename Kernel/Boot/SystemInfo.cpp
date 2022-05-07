#include <Boot/SystemInfo.h>
#include <Library/BasicFunctions.hpp>

const int SystemInfoLines=3;
static const char* SystemInfo[SystemInfoLines]=
{
	"PAL_OperatingSystem",
	"Version 0.1",
	"By: qianpinyi&&peaceSheep"
};

void PrintSystemInfo()
{
	using namespace POS;
	for (int i=0;i<SystemInfoLines;++i)
		Puts(SystemInfo[i]),Putchar('\n');
}
