#include <AsmDef.h>
#include <Riscv.h>

	.section .text.entry
	.globl _start
_start:
	lui     t0, %hi(boot_page_table_sv39)
	li      t1, 0xffffffffc0000000 - 0x80000000
	sub		t0,	t0,	t1
	srli	t0,	t0,	12
	li		t1,	8<<60
	or		t0,	t0,	t1
	csrw	satp, t0
	sfence.vma
	
	lui sp, %hi(bootstacktop)

	lui t0, %hi(main)
	addi t0, t0, %lo(main)
	jalr t0

halt:
	j halt

loop:
	j loop

	.section .bss.stack
	.align PGSHIFT
	.global bootstack
bootstack:
	.space KSTACKSIZE
	.global bootstacktop
bootstacktop:

	.section .data
	.align PGSHIFT
	.global boot_page_table_sv39
boot_page_table_sv39:
	.zero REGBYTES*511
	.quad (0x80000 << 10) | 0xcf
	
