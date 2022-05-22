#include <Trap/Syscall.hpp>
#include <Library/Kout.hpp>
#include <Process/Process.hpp>
#include <Trap/Interrupt.hpp>
using namespace POS;

int Syscall_Putchar(char ch)
{
	Putchar(ch);
	return 0;
}

int Syscall_Getchar()
{
	return Getchar();
}

int Syscall_Getputchar()
{
	return Getputchar();
}

int Syscall_Exit(int re)
{
	Process *cur=POS_PM.Current();
	cur->Exit(re);
	POS_PM.Schedule();
	kout[Fault]<<"Syscall_Exit: Reached unreachable branch!"<<endl;
	return ERR_Unknown;
}

int Syscall_Clone(Uint64 flags,PtrInt stack,PID faPID,Uint64 tls,PID childPID)
{
	kout[Fault]<<"Syscall_Clone is not usable!"<<endl;
	return -1;
}

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
