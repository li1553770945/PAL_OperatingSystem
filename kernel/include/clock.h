//时钟相关
#ifndef _CLOCK_H_
#define _CLOCK_H_
#include "defs.h"
void sbi_set_timer(unsigned long long stime_value);//OpenSBI提供的sbi_set_timer()接口，可以传入一个时刻，让它在那个时刻触发一次时钟中断
void clock_set_next_event(void);
void clock_init(void);//初始化时钟中断
static inline uint64_t get_time(void);//获取当前时间，方便设置下一个时钟中断
extern volatile uint64_t ticks;
#endif
