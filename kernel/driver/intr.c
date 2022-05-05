#include "intr.h"
#include "../include/riscv.h"

/* intr_enable - enable irq interrupt, 设置sstatus的Supervisor中断使能位 */
void intr_enable(void) 
{ 
    set_csr(sstatus, SSTATUS_SIE); 
}
/* intr_disable - disable irq interrupt */
void intr_disable(void)
{
    clear_csr(sstatus, SSTATUS_SIE);
} 