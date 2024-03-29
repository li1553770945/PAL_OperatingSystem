#include <Riscv.h>

.text
.globl KernelThreadEntry
KernelThreadEntry:
	csrw sstatus, s2
	la a0,KernelThreadEntry2
	csrw sepc, a0
	sret

.globl KernelThreadEntry2
KernelThreadEntry2:
	move a0,s1
	jalr s0
	jal KernelThreadExit

.globl UserThreadEntry
UserThreadEntry:
	beqz s0,UserThreadEntry2
	move a0,s1
	jalr s0
	jal SwitchToUserStat
UserThreadEntry2:
	j __trapret

.globl ProcessSwitchContext
ProcessSwitchContext:
	STORE ra,  0*REGBYTES(a0)
	STORE sp,  1*REGBYTES(a0)
	STORE s0,  2*REGBYTES(a0)
	STORE s1,  3*REGBYTES(a0)
	STORE s2,  4*REGBYTES(a0)
	STORE s3,  5*REGBYTES(a0)
	STORE s4,  6*REGBYTES(a0)
	STORE s5,  7*REGBYTES(a0)
	STORE s6,  8*REGBYTES(a0)
	STORE s7,  9*REGBYTES(a0)
	STORE s8,  10*REGBYTES(a0)
	STORE s9,  11*REGBYTES(a0)
	STORE s10, 12*REGBYTES(a0)
	STORE s11, 13*REGBYTES(a0)
	
	LOAD ra,   0*REGBYTES(a1)
	LOAD sp,   1*REGBYTES(a1)
	LOAD s0,   2*REGBYTES(a1)
	LOAD s1,   3*REGBYTES(a1)
	LOAD s2,   4*REGBYTES(a1)
	LOAD s3,   5*REGBYTES(a1)
	LOAD s4,   6*REGBYTES(a1)
	LOAD s5,   7*REGBYTES(a1)
	LOAD s6,   8*REGBYTES(a1)
	LOAD s7,   9*REGBYTES(a1)
	LOAD s8,   10*REGBYTES(a1)
	LOAD s9,   11*REGBYTES(a1)
	LOAD s10,  12*REGBYTES(a1)
	LOAD s11,  13*REGBYTES(a1)
	
	ret
