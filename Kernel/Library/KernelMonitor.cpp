#include <Error.hpp>
#include <Library/Kout.hpp>
#include <Trap/Interrupt.hpp>
#include <Library/String/StringTools.hpp>
#include <Process/Process.hpp>
#include <HAL/Disk.hpp>
#include <File/FileSystem.hpp>

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
		else if (strComp(cmd,"continue")==0)
			break;
		else if (strComp(cmd,"ProcessStat")==0)
		{
			for (int i=0;i<POS_PM.ProcessCount;++i)
				kout<<LightYellow<<i<<":"<<POS_PM.Processes[i].stat<<endl;
		}
		else if (strComp(cmd,"CurrentProcess")==0)
			kout<<LightYellow<<POS_PM.Current()->GetPID()<<endl;
		else if (strComp(cmd,"sstatus")==0)
			kout<<LightYellow<<(void*)read_csr(sstatus)<<endl;
		else if (strComp(cmd,"sie")==0)
			kout<<LightYellow<<(void*)read_csr(sie)<<endl;
		else if (strComp(cmd,"sepc")==0)
			kout<<LightYellow<<(void*)read_csr(sepc)<<endl;
		else if (strComp(cmd,"*sepc")==0)
			kout<<LightYellow<<(void*)*(Uint32*)read_csr(sepc)<<endl;
		else if (strComp(cmd,"scause")==0)
			kout<<LightYellow<<(void*)read_csr(scause)<<endl;
		else if (strComp(cmd,"stvec")==0)
			kout<<LightYellow<<(void*)read_csr(stvec)<<endl;
		else if (strComp(cmd,"stval")==0)
			kout<<LightYellow<<(void*)read_csr(stval)<<endl;
		else if (strComp(cmd,"sscratch")==0)
			kout<<LightYellow<<(void*)read_csr(sscratch)<<endl;
		else if (strComp(cmd,"sip")==0)
			kout<<LightYellow<<(void*)read_csr(sip)<<endl;
		else if (strComp(cmd,"sp")==0)
		{
			RegisterData sp;
			asm volatile("mv %0,sp":"=r"(sp));
			kout<<LightYellow<<(void*)sp<<" ["<<POS_PM.Current()->Stack<<","<<POS_PM.Current()->Stack+POS_PM.Current()->StackSize<<")"<<endl;
		}
		else if (strComp(cmd,"PrintStack")==0)
		{
			RegisterData sp;
			asm volatile("mv %0,sp":"=r"(sp));
			kout<<LightYellow<<DataWithSize((void*)sp,(PtrInt)POS_PM.Current()->Stack+POS_PM.Current()->StackSize-sp)<<endl;
		}
		else if (strComp(cmd,"CurrentVMRs")==0)
		{
			for (VirtualMemoryRegion *vmr=POS_PM.Current()->GetVMS()->VmrHead.Nxt();vmr;vmr=vmr->Nxt())
				kout<<LightYellow<<"["<<(void*)vmr->Start<<","<<(void*)vmr->End<<") "<<(void*)vmr->Flags<<endl;
		}
		else if (strComp(cmd,"CallingStack")==0)
			CallingStackController::PrintCallingStack(POS_PM.Current()->CallingStack);
		else if (strComp(cmd,"CallingStack0")==0)
			CallingStackController::PrintCallingStack(POS_PM.GetProcess(0)->CallingStack);
		else if (strComp(cmd,"CallingStack1")==0)
			CallingStackController::PrintCallingStack(POS_PM.GetProcess(1)->CallingStack);
		else if (strComp(cmd,"CallingStack2")==0)
			CallingStackController::PrintCallingStack(POS_PM.GetProcess(2)->CallingStack);
		else if (strComp(cmd,"CallingStack3")==0)
			CallingStackController::PrintCallingStack(POS_PM.GetProcess(3)->CallingStack);
		else if (strComp(cmd,"CallingStack4")==0)
			CallingStackController::PrintCallingStack(POS_PM.GetProcess(4)->CallingStack);
		else if (strComp(cmd,"CurrentFileTable")==0)
		{
			FileHandle *fh=POS_PM.Current()->GetFileHandleFromFD(0);
			while (fh!=nullptr)
			{
				char *path=fh->Node()->GetPath<0>();
				kout<<LightYellow<<fh->GetFD()<<"("<<fh<<"):"<<path<<endl;
				delete path;
				fh=fh->Nxt();
			}
		}
		else if (strComp(cmd,"FileTree")==0)
		{
			auto PrintVFSM=[](auto &self,const char *path,int dep=0)->void
			{
				kout<<LightYellow<<dep<<": "<<path<<endl;
				char *buffer[16];
				int skip=0;
				while (1)
				{
					int cnt=VFSM.GetAllFileIn(path,buffer,16,skip);
					for (int i=0;i<cnt;++i)
					{
						char *child=strSplice(path,dep==0?"":"/",buffer[i]);
						if (buffer[i][0]!='.')
							self(self,child,dep+1);
						Kfree(child);
						Kfree(buffer[i]);
					}
					if (cnt<16)
						break;
					else skip+=16;
				}
			};
			PrintVFSM(PrintVFSM,"/");
		}
		else if (strComp(cmd,"FreePages")==0)
			kout<<LightYellow<<POS_PMM.GetFreePageNum()<<endl;
		else if (strComp(cmd,"")==0)
		{

		}
		else if (strComp(cmd,"")==0)
		{

		}
		else if (strComp(cmd,"")==0)
		{

		}
		else if (strComp(cmd,"")==0)
		{

		}
		else kout<<LightRed<<"Unknown command!"<<endl;
	}
}
