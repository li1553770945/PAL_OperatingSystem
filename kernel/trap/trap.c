#include "../include/trap.h"
#include "../include/riscv.h"
#include "../include/printk.h"
#include "../include/clock.h"

#define TICK_NUM 100
static void print_ticks() {
    putsk("ticks\n");
}

void idt_init(void)
 {
    extern void __alltraps(void);
    //约定：若中断前处于S态，sscratch为0
    //若中断前处于U态，sscratch存储内核栈地址
    //那么之后就可以通过sscratch的数值判断是内核态产生的中断还是用户态产生的中断
    //我们现在是内核态所以给sscratch置零
    write_csr(sscratch, 0);
    //我们保证__alltraps的地址是四字节对齐的，将__alltraps这个符号的地址直接写到stvec寄存器
    write_csr(stvec, &__alltraps);
}


 void interrupt_handler(struct trapframe *tf) {
    intptr_t cause = (tf->cause << 1) >> 1; //抹掉scause最高位代表“这是中断不是异常”的1
    switch (cause) {
        case IRQ_U_SOFT:
            putsk("User software interrupt\n");
            break;
        case IRQ_S_SOFT:
            putsk("Supervisor software interrupt\n");
            break;
        case IRQ_H_SOFT:
            putsk("Hypervisor software interrupt\n");
            break;
        case IRQ_M_SOFT:
            putsk("Machine software interrupt\n");
            break;
        case IRQ_U_TIMER:
            putsk("User software interrupt\n");
            break;
        case IRQ_S_TIMER:
            //时钟中断
            clock_set_next_event();
            if (++ticks % TICK_NUM == 0) {
                print_ticks();
            }
            break;
        case IRQ_H_TIMER:
            putsk("Hypervisor software interrupt\n");
            break;
        case IRQ_M_TIMER:
            putsk("Machine software interrupt\n");
            break;
        case IRQ_U_EXT:
            putsk("User software interrupt\n");
            break;
        case IRQ_S_EXT:
            putsk("Supervisor external interrupt\n");
            break;
        case IRQ_H_EXT:
            putsk("Hypervisor software interrupt\n");
            break;
        case IRQ_M_EXT:
            putsk("Machine software interrupt\n");
            break;
        default:
            putsk("unknown interrupt");
            break;
    }
}

void exception_handler(struct trapframe *tf) {
    switch (tf->cause) {
        case CAUSE_MISALIGNED_FETCH:
            break;
        case CAUSE_FAULT_FETCH:
            break;
        case CAUSE_ILLEGAL_INSTRUCTION:
            break;
        case CAUSE_BREAKPOINT:
            break;
        case CAUSE_MISALIGNED_LOAD:
            break;
        case CAUSE_FAULT_LOAD:
            break;
        case CAUSE_MISALIGNED_STORE:
            break;
        case CAUSE_FAULT_STORE:
            break;
        case CAUSE_USER_ECALL:
            break;
        case CAUSE_SUPERVISOR_ECALL:
            break;
        case CAUSE_HYPERVISOR_ECALL:
            break;
        case CAUSE_MACHINE_ECALL:
            break;
        default:
            putsk("unknown exception");
            break;
    }
}
void trap(struct trapframe *tf) 

{  //scause的最高位是1，说明trap是由中断引起的
    if ((intptr_t)tf->cause < 0) {
        // 中断
        interrupt_handler(tf);
    } 
    else
    {
        // 异常
        exception_handler(tf);
    }
    
}