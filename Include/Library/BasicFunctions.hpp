#ifndef POS_BASICFUNCTIONS_HPP
#define POS_BASICFUNCTIONS_HPP

#include "../SBI.h"
#include "TemplateTools.hpp"

namespace POS
{
	inline void Putchar(char ch)
	{
		SBI_PUTCHAR(ch);
	}
	
	inline void Puts(const char *s)
	{
		if (s==nullptr)//??
			return;
		while (*s)
			Putchar(*s++);
	}
	
	inline char Getchar()//Will block
	{
		constexpr bool IsRustSBI=0;
		if (IsRustSBI)
			return SBI_GETCHAR();
		else
		{
			int ch;
			while ((ch=SBI_GETCHAR())==-1);
			return ch;
		}
	}
	
	inline char Getputchar()
	{
		char ch=Getchar();
		Putchar(ch);
		return ch;
	}
	
	inline int Getline(char dst[],int bufferSize)
	{
		int i=0;
		while (i<bufferSize)
		{
			char ch=Getchar();
			if (InThisSet(ch,'\n','\r','\0',-1))//??
			{
				Putchar('\n');
				dst[i++]=0;
				break;
			}
			else Putchar(ch);
			dst[i++]=ch;
//			if (ch=='\b')
//				--i;
		}
		return i;
	}
	
	#define DoNothing 0
};

#endif
