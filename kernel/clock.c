#include "include/clock.h"
#include "include/defs.h"
#include "include/sbi.h"
#include "include/riscv.h"
#include "include/printk.h"

//volatile告诉编译器这个变量可能在其他地方被瞎改一通，所以编译器不要对这个变量瞎优化
volatile uint64_t ticks;

//对64位和32位架构，读取time的方法是不同的
//32位架构下，需要把64位的time寄存器读到两个32位整数里，然后拼起来形成一个64位整数
//64位架构简单的一句rdtime就可以了
//__riscv_xlen是gcc定义的一个宏，可以用来区分是32位还是64位。

static inline uint64_t get_time(void)//返回当前时间
{
#if __riscv_xlen == 64
    uint64_t n;
    __asm__ __volatile__("rdtime %0" : "=r"(n));
    return n;
#else
    uint32_t lo, hi, tmp;
    __asm__ __volatile__(
        "1:\n"
        "rdtimeh %0\n"
        "rdtime %1\n"
        "rdtimeh %2\n"
        "bne %0, %2, 1b"
        : "=&r"(hi), "=&r"(lo), "=&r"(tmp));
    return ((uint64_t)hi << 32) | lo;
#endif
}


//  一个时间片的时间，对于qemu，这个数值每+1，时间+100ns
static uint64_t timebase = 1000;

void clock_init(void) 
{
    // sie这个CSR可以单独使能/禁用某个来源的中断。默认时钟中断是关闭的
    // 所以我们要在初始化的时候，使能时钟中断
    set_csr(sie, MIP_STIP); // enable timer interrupt in sie
    //设置第一个时钟中断事件
    clock_set_next_event();
    // 初始化一个计数器,注意：该计数器在clock.h声明，全局可用
    ticks = 0;
    putsk("init timer interrupts suuccess\n");
}

//设置时钟中断：timer的数值变为当前时间 + timebase 后，触发一次时钟中断
//对于QEMU, timer增加1，过去了10^-7 s， 也就是100ns
void clock_set_next_event(void) 
{
    sbi_set_timer(get_time() + timebase); 
}