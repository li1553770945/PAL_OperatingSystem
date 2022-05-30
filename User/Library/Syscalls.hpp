#ifndef POS_USERSYSCALLS_HPP
#define POS_USERSYSCALLS_HPP

#include <SyscallID.hpp>
#include <Types.hpp>

inline RegisterData POS_Syscall(RegisterData ID,
					   RegisterData a0=0,
					   RegisterData a1=0,
					   RegisterData a2=0,
					   RegisterData a3=0,
					   RegisterData a4=0,
					   RegisterData a5=0)
{
	RegisterData re;
	asm volatile
	(
		"ld a0, %1\n"
		"ld a1, %2\n"
		"ld a2, %3\n"
		"ld a3, %4\n"
		"ld a4, %5\n"
		"ld a5, %6\n"
		"ld a7, %7\n"
		"ecall\n"
		"sd a0, %0"
		:"=m" (re)
		:"m"(a0),"m"(a1),"m"(a2),"m"(a3),"m"(a4),"m"(a5),"m"(ID)
		:"memory"
	);
	return re;
}

inline void Sys_Putchar(char ch) {POS_Syscall(SYS_Putchar,ch);}
inline char Sys_Getchar() {return POS_Syscall(SYS_Getchar);}
inline char Sys_Getputchar() {return POS_Syscall(SYS_Getputchar);}
inline void Sys_Exit(int re) {POS_Syscall(SYS_Exit,re);}
inline PID Sys_Fork() {return POS_Syscall(SYS_Fork);}
inline PID Sys_GetPID() {return POS_Syscall(SYS_GetPID);}

#endif
