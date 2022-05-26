#include <Trap/Syscall.hpp>
#include <Library/Kout.hpp>
#include <Process/Process.hpp>
#include <Trap/Interrupt.hpp>
using namespace POS;

inline void Syscall_Putchar(char ch)
{Putchar(ch);}

inline char Syscall_Getchar()
{return Getchar();}

inline char Syscall_Getputchar()
{return Getputchar();}

void Syscall_Exit(int re)
{
	Process *cur=POS_PM.Current();
	cur->Exit(re);
	POS_PM.Schedule();
	kout[Fault]<<"Syscall_Exit: Reached unreachable branch!"<<endl;
}

PID Syscall_Clone(Uint64 flags,PtrInt stack,PID faPID,Uint64 tls,PID childPID)
{
	kout[Fault]<<"Syscall_Clone is not usable yet!"<<endl;
	return Process::InvalidPID;
}

PID Syscall_Fork(TrapFrame *tf)
{
	Process *cur=POS_PM.Current();
	while (1)
	{
		ErrorType err=ForkServer.RequestFork(cur,tf);
//		kout[Debug]<<"ERRRRRRRRRRRRRRRRRRRRRRRRRRR "<<err<<endl;
		if (err==ERR_None)
//		{
//			Process *c=POS_PM.Current();
//			kout[Debug]<<"OKKKKKKKKKKKKKKKKKKKK "<<c->GetPID()<<endl;
//			register volatile RegisterData sp asm("sp");
//			kout[Debug]<<"CurrentSP "<<(void*)sp<<endl;
//			kout[Debug]<<"## "<<&err<<endl;
//			if (c==cur)
				return 0;
//			else
//			{
//				kout[Fault]<<"???"<<endl;
//			}
//			return cur==c?0:c->GetPID();//??
//		}
		else if (err!=ERR_BusyForking)
			return Process::InvalidPID;
//		kout[Fault]<<"Request again??"<<endl;
	}
}

inline  PID Syscall_GetPID()
{return POS_PM.Current()->GetPID();}

ErrorType TrapFunc_Syscall(TrapFrame *tf)
{
	InterruptStackAutoSaverBlockController isas;//??
//	kout[Test]<<"Syscall "<<tf->reg.a7<<" | "<<tf->reg.a0<<" "<<tf->reg.a1<<" "<<tf->reg.a2<<" "<<tf->reg.a3<<" "<<tf->reg.a4<<" "<<tf->reg.a5<<endl;
	switch (tf->reg.a7)
	{
		case SYS_Putchar:
			Syscall_Putchar(tf->reg.a0);
			break;
		case SYS_Getchar:
			tf->reg.a0=Syscall_Getchar();
			break;
		case SYS_Getputchar:
			tf->reg.a0=Syscall_Getputchar();
			break;
		case SYS_Exit:
			Syscall_Exit(tf->reg.a0);
			break;
		case SYS_Clone:
			tf->reg.a0=Syscall_Clone(tf->reg.a0,tf->reg.a1,tf->reg.a2,tf->reg.a3,tf->reg.a4);
			break;
		case SYS_Fork:
			Syscall_Fork(tf);
//			kout<<"TF "<<tf<<endl;
//			kout<<DataWithSizeUnited(tf,sizeof(TrapFrame),sizeof(RegisterData))<<endl;
//			kout<<"SP EPC "<<(void*)tf->reg.sp<<" "<<(void*)tf->epc+4<<endl;
			break;
		case SYS_GetPID:
			tf->reg.a0=Syscall_GetPID();
			break;
		default:
		{
			Process *cur=POS_PM.Current();
			if (cur->IsKernelProcess())
				kout[Fault]<<"TrapFunc_Syscall: Unknown syscall "<<tf->reg.a7<<" from kernel process "<<cur->GetPID()<<"!"<<endl;
			else
			{
				kout[Error]<<"TrapFunc_Syscall: Unknown syscall "<<tf->reg.a7<<" from user process "<<cur->GetPID()<<"!"<<endl;
				cur->Exit(Process::Exit_BadSyscall);
				POS_PM.Schedule();
			}
			break;
		}
	}
	tf->epc+=4;
	return ERR_None;
}
