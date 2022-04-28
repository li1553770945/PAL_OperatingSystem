#include "sbi.h"

void putchark(char ch)
{
	SBI_PUTCHAR(ch);
}
void putsk(char *s)
{
	while (*s)
		putchark(*s++);
}