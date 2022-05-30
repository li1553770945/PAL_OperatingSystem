#include <Error.hpp>
#include <Library/Kout.hpp>
#include <Trap/Interrupt.hpp>
#include <Library/String/StringTools.hpp>

using namespace POS;

void KernelFaultSolver()
{
	static bool flag=0;
	if (flag==1)
	{
		kout<<LightRed<<"[Panic!!!] Kernel fault in KernelFaultSolver! System shutdown."<<endl;
		SBI_SHUTDOWN();
//		while(1);
	}
	InterruptStackAutoSaver isas;
	while (1)
	{
		kout<<LightRed<<"<KernelMonitor>:"<<Reset;
		char cmd[256];
		Getline(cmd,256);
		if (strComp(cmd,"shutdown")==0)
			SBI_SHUTDOWN();
		else kout<<LightRed<<"Unknown command!"<<endl;
	}
}
