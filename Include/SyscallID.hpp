#ifndef POS_SYSCALLID_HPP
#define POS_SYSCALLID_HPP

enum SyscallID
{
	SYS_Putchar=1,
	SYS_Getchar=2,
	SYS_Getputchar=3,
	SYS_Exit=4,
	SYS_Fork=5,
	SYS_GetPID=6,
	
	SYS_getcwd		=17,
	SYS_pipe2		=59,
	SYS_dup			=23,
	SYS_dup3		=24,
	SYS_chdir		=49,
	SYS_openat		=56,
	SYS_close		=57,
	SYS_getdents64	=61,
	SYS_read		=63,
	SYS_write		=64,
	SYS_linkat		=37,
	SYS_unlinkat	=35,
	SYS_mkdirat		=34,
	SYS_umount2		=39,
	SYS_mount		=40,
	SYS_fstat		=80,
	SYS_clone		=220,
	SYS_execve		=221,
	SYS_wait4		=260,
	SYS_exit		=93,
	SYS_getppid		=173,
	SYS_getpid		=172,
	SYS_brk			=214,
	SYS_munmap		=215,
	SYS_mmap		=222,
	SYS_times		=153,
	SYS_uname		=160,
	SYS_sched_yeild	=124,
	SYS_gettimeofday=169,
	SYS_nanosleep	=101,
};

#endif
