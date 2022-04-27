	.section .text.entry
	.globl _start
_start:
	la sp,stack_top
	call main

halt:
	j halt

loop:
	j loop

	.section .bss.stack
	.align 12
	.global stack_top

stack_top:
	.space 4096*4
	.global stack_top
