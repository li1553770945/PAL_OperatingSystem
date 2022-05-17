#ifndef POS_ERROR_HPP
#define POS_ERROR_HPP

#include "Trap/Interrupt.h"
#include "Library/BasicFunctions.hpp"

enum
{
	ERR_None=0,
	ERR_Unknown,
	ERR_Todo,
	ERR_KmallocFailed,
	ERR_AccessInvalidVMR,
	ERR_AccessPermissionViolate,
};

inline void KernelFaultSolver()
{
	while (1);
}

inline void Panic(const char *msg=nullptr)
{
	InterruptStackAutoSaver isas;
	POS::Puts("[Panic!!!] ");
	POS::Puts(msg);
	POS::Puts("\n");
	KernelFaultSolver();
}

inline void Assert(bool flag,const char *msg=nullptr)
{
	if (!flag)
		Panic(msg);
}

#define ASSERT(x,s)		\
{						\
	if (!(x))			\
		Panic(s);	\
}

#endif
