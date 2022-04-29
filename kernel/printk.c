#include "include/printk.h"
#include "include/sbi.h"
void putchark(char ch) 
{
     sbi_console_putchar(ch);
}

void putsk(const char *str) 
{
    while (*str != '\0') 
    {
        putchark(*str);
        str++;
    }
}

void printk(const char *str,...)
{
    
}