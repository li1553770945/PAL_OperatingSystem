#include "sbi.h"

void Putchar(char ch)
{
	SBI_PUTCHAR(ch);
}

void Puts(char *s)
{
	while (*s)
		Putchar(*s++);
}

void main()
{
	Puts("Welcome to PAL_OperatingSystem\n");
	Puts("Hello,World\n");
	while(1);
}
