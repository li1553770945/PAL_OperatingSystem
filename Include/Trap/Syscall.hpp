#ifndef POS_SYSCALL_HPP
#define POS_SYSCALL_HPP

#include "Trap.hpp"
#include "../SyscallID.hpp"//Need improve...

//int Syscall_Putchar(char ch);
//int Syscall_Getchar();
//int Syscall_Getputchar();
//int Syscall_Exit(int re);

ErrorType TrapFunc_Syscall(TrapFrame *tf);

#endif
