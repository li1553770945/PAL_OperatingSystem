#include <Trap/Interrupt.h>
#include <Riscv.h>

void InterruptEnable()
{
	set_csr(sstatus,SSTATUS_SIE);
}

void InterruptDisable()
{
	clear_csr(sstatus,SSTATUS_SIE);
}
