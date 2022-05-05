#include "trap/trap.h"
#include "driver/intr.h"
#include "include/init.h"
#include "include/printk.h"
#include "include/clock.h"
void print_kernel_info()
{
    putsk("Welcome to PAL Operation System!\n");
}
void init()
{
    print_kernel_info();
    //trap.h的函数，初始化中断
    idt_init();  // init interrupt descriptor table
    //clock.h的函数，初始化时钟中断
    clock_init();  
    //intr.h的函数，使能中断
    intr_enable();  
}
