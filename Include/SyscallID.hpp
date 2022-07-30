#ifndef POS_SYSCALLID_HPP
#define POS_SYSCALLID_HPP

enum SyscallID
{
	SYS_Putchar=-1,
	SYS_Getchar=-2,
	SYS_Getputchar=-3,
	SYS_Exit=-4,
	SYS_Fork=-5,
	SYS_GetPID=-6,
	SYS_Rest=-7,
	
	SYS_getcwd			=17,
	SYS_dup				=23,
	SYS_dup3			=24,
	SYS_fcntl			=25,
	SYS_ioctl			=29,
	SYS_mkdirat			=34,
	SYS_unlinkat		=35,
	SYS_linkat			=37,
	SYS_umount2			=39,
	SYS_mount			=40,
	SYS_statfs			=43, 
	SYS_chdir			=49,
	SYS_openat			=56,
	SYS_close			=57,
	SYS_pipe2			=59,
	SYS_getdents64		=61,
	SYS_lseek			=62,
	SYS_read			=63,
	SYS_write			=64,
	SYS_readv			=65,
	SYS_writev			=66,
	SYS_pread64			=67,
	SYS_pwrite64		=68,
	SYS_preadv			=69,
	SYS_pwritev			=70,
	SYS_newfstatat		=79,
	SYS_fstat			=80,
	SYS_utimensat		=88,
	SYS_exit			=93,
	SYS_exit_group		=94,
	SYS_set_tid_address	=96,
	SYS_futex			=98,
	SYS_get_robust_list	=100,
	SYS_nanosleep		=101,
	SYS_clock_gettime	=113,
	SYS_sched_yeild		=124,
	SYS_sigaction		=134,
	SYS_sigprocmask		=135,
	SYS_sigtimedwait	=137,
	SYS_times			=153,
	SYS_uname			=160,
	SYS_gettimeofday	=169,
	SYS_getpid			=172,
	SYS_getppid			=173,
	SYS_geteuid			=175,
	SYS_getegid			=177,
	SYS_gettid			=178,
	SYS_socket			=198,
	SYS_bind			=200, 
	SYS_listen			=201,
	SYS_accept			=202,
	SYS_connect			=203,
	SYS_getsockname		=204,
	SYS_sendto			=206,
	SYS_recvfrom		=207,
	SYS_setsockopt		=208,
	SYS_brk				=214,
	SYS_munmap			=215,
	SYS_mmap			=222,
	SYS_clone			=220,
	SYS_execve			=221,
	SYS_mprotect		=226,
	SYS_wait4			=260,
	SYS_prlimit64		=261,
	SYS_membarrier		=283,
};

inline const char *SyscallName(int sys)
{
	switch (sys)
	{
		case SYS_Putchar:			return "SYS_Putchar";
		case SYS_Getchar:			return "SYS_Getchar";
		case SYS_Getputchar:		return "SYS_Getputchar";
		case SYS_Exit:				return "SYS_Exit";
		case SYS_Fork:				return "SYS_Fork";
		case SYS_GetPID:			return "SYS_GetPID";
		case SYS_Rest:				return "SYS_Rest";
	
		case SYS_getcwd:			return "SYS_getcwd";
		case SYS_dup:				return "SYS_dup";
		case SYS_dup3:				return "SYS_dup3";
		case SYS_fcntl:				return "SYS_fcntl";
		case SYS_ioctl:				return "SYS_ioctl";
		case SYS_mkdirat:			return "SYS_mkdirat";
		case SYS_unlinkat:			return "SYS_unlinkat";
		case SYS_linkat:			return "SYS_linkat";
		case SYS_umount2:			return "SYS_umount2";
		case SYS_mount:				return "SYS_mount";
		case SYS_statfs:			return "SYS_statfs";
		case SYS_chdir:				return "SYS_chdir";
		case SYS_openat:			return "SYS_openat";
		case SYS_close:				return "SYS_close";
		case SYS_pipe2:				return "SYS_pipe2";
		case SYS_getdents64:		return "SYS_getdents64";
		case SYS_lseek:				return "SYS_lseek";
		case SYS_read:				return "SYS_read";
		case SYS_write:				return "SYS_write";
		case SYS_readv:				return "SYS_readv";
		case SYS_writev:			return "SYS_writev";
		case SYS_pread64:			return "SYS_pread64";
		case SYS_pwrite64:			return "SYS_pwrite64";
		case SYS_preadv:			return "SYS_preadv";
		case SYS_pwritev:			return "SYS_pwritev";
		case SYS_newfstatat:		return "SYS_newfstatat";
		case SYS_fstat:				return "SYS_fstat";
		case SYS_utimensat:			return "SYS_utimensat";
		case SYS_exit:				return "SYS_exit";
		case SYS_exit_group:		return "SYS_exit_group";
		case SYS_set_tid_address:	return "SYS_set_tid_address";
		case SYS_futex:				return "SYS_futex";
		case SYS_get_robust_list:	return "SYS_get_robust_list";
		case SYS_nanosleep:			return "SYS_nanosleep";
		case SYS_clock_gettime:		return "SYS_clock_gettime";
		case SYS_sched_yeild:		return "SYS_sched_yeild";
		case SYS_sigaction:			return "SYS_sigaction";
		case SYS_sigprocmask:		return "SYS_sigprocmask";
		case SYS_sigtimedwait:		return "SYS_sigtimedwait";
		case SYS_times:				return "SYS_times";
		case SYS_uname:				return "SYS_uname";
		case SYS_gettimeofday:		return "SYS_gettimeofday";
		case SYS_getpid:			return "SYS_getpid";
		case SYS_getppid:			return "SYS_getppid";
		case SYS_geteuid:			return "SYS_geteuid";
		case SYS_getegid:			return "SYS_getegid";
		case SYS_gettid:			return "SYS_gettid";
		case SYS_socket:			return "SYS_socket";
		case SYS_bind:				return "SYS_bind";
		case SYS_listen:			return "SYS_listen";
		case SYS_accept:			return "SYS_accept";
		case SYS_connect:			return "SYS_connect";
		case SYS_getsockname:		return "SYS_getsockname";
		case SYS_sendto:			return "SYS_sendto";
		case SYS_recvfrom:			return "SYS_recvfrom";
		case SYS_setsockopt:		return "SYS_setsockopt";
		case SYS_brk:				return "SYS_brk";
		case SYS_munmap:			return "SYS_munmap";
		case SYS_mmap:				return "SYS_mmap";
		case SYS_clone:				return "SYS_clone";
		case SYS_execve:			return "SYS_execve";
		case SYS_mprotect:			return "SYS_mprotect";
		case SYS_wait4:				return "SYS_wait4";
		case SYS_prlimit64:			return "SYS_prlimit64";
		case SYS_membarrier:		return "SYS_membarrier";
		default:					return ""; 
	}
}

#endif
