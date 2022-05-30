.section .text.entry
.globl _start
_start:
	call _UserMain

loop:	#Error loop
	j loop
