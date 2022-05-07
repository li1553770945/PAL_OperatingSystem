#include "include/printk.h"
#include "include/init.h"
int main()
{
	init();
	int a = 1;
	printk("123%d",a);
	while(1);
	return 0;
}
