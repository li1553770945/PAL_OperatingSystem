#ifndef POS_BASICFUNCTIONS_HPP
#define POS_BASICFUNCTIONS_HPP

#include "../SBI.h"

namespace POS
{
	inline void Putchar(char ch)
	{
		SBI_PUTCHAR(ch);
	}
	
	inline void Puts(const char *s)
	{
		while (*s)
			Putchar(*s++);
	}
	
	#define DoNothing 0
};

#endif
