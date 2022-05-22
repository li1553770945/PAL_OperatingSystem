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
	move	s3,t0
	
	addi	a0,zero,65
	addi	a7,zero,1
	ecall
	
	csrr	s1,satp
	csrw	satp,s3
	li		s2,10;
	move	s1,s3
FLAGA:
	remu	a0,s1,s2
	addi	a0,a0,48
	addi	a7,zero,1
	ecall
	divu	s1,s1,s2
	bnez	s1,FLAGA
	
	
	addi	a0,zero,32
	addi	a7,zero,1
	ecall
	
	csrr	s1,satp
	csrw	satp,s3
FLAGB:
	remu	a0,s1,s2
	addi	a0,a0,48
	addi	a7,zero,1
	ecall
	divu	s1,s1,s2
	bnez	s1,FLAGB
	
	sfence.vma
	
	addi	a0,zero,66
	addi	a7,zero,1
	ecall

#	la sp, bootstacktop
	lui sp, %hi(bootstacktop)

	lui t0, %hi(mymain)
	addi t0, t0, %lo(mymain)
#	la t0,main
	jr t0
#	call main

mymain:
	call main
	

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
	.zero REGBYTES*2
	.quad (0x80000 << 10) | 0xcf
	.zero REGBYTES*508
	.quad (0x80000 << 10) | 0xcf
	
