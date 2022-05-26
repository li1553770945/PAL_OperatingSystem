#ifndef POS_SYSCALL_HPP
#define POS_SYSCALL_HPP

#include "Trap.hpp"
#include "../SyscallID.hpp"//Need improve...

ErrorType TrapFunc_Syscall(TrapFrame *tf);

#endif
