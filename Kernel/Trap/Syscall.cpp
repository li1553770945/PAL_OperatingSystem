#include <Trap/Syscall.hpp>
#include <Library/Kout.hpp>
#include <Process/Process.hpp>
#include <Trap/Interrupt.hpp>
#include <Process/Synchronize.hpp>
#include <Library/String/StringTools.hpp>
#include <File/FileSystem.hpp>
#include <Process/ELF.hpp>
#include <File/FileNodeEX.hpp>
using namespace POS;

inline void Syscall_Putchar(char ch)
{Putchar(ch);}

inline char Syscall_Getchar()
{return Getchar();}

inline char Syscall_Getputchar()
{return Getputchar();}

void Syscall_Exit(int re)
{
	Process *cur=POS_PM.Current();
	cur->Exit(re);
	POS_PM.Schedule();
	kout[Fault]<<"Syscall_Exit: Reached unreachable branch!"<<endl;
}

PID Syscall_Fork(TrapFrame *tf)
{
	Process *cur=POS_PM.Current();
	while (1)
	{
		ErrorType err=ForkServer.RequestFork(cur,tf);
		if (err==ERR_None)
//			Process *c=POS_PM.Current();
//			if (c==cur)
				return tf->reg.a0; 
//			else
//			{
//				kout[Fault]<<"???"<<endl;
//			}
//			return cur==c?0:c->GetPID();//??
//		}
		else if (err!=ERR_BusyForking)
			return Process::InvalidPID;
//		kout[Fault]<<"Request again??"<<endl;
	}
}


inline char* Syscall_getcwd(char *buf,Uint64 size)
{
	if (buf==nullptr)
		kout[Warning]<<"Syscall_getcwd: buf is nullptr is not supported!"<<endl;
	else
	{
		const char *cwd=POS_PM.Current()->GetCWD();
		Uint64 len=strLen(cwd);
		VirtualMemorySpace::EnableAccessUser();
		if (len>0&&len<size)
			strCopy(buf,cwd);
		VirtualMemorySpace::DisableAccessUser();
	}
	return buf;
}

inline int Syscall_pipe2(int *fd,int flags)
{
	Process *cur=POS_PM.Current();
	PipeFileNode *pipe=new PipeFileNode();
	if (pipe==nullptr)
		return -1;
	FileHandle *fh0=new FileHandle(pipe,FileHandle::F_Read),
			   *fh1=new FileHandle(pipe,FileHandle::F_Write);//??
	if (InThisSet(nullptr,fh0,fh1))
	{
		if (fh0==nullptr&&fh1==nullptr)
			delete pipe;
		else if (fh0==nullptr)
			delete fh1;
		else delete fh0;
		return -1;
	}
	VirtualMemorySpace::EnableAccessUser();
	fh0->BindToProcess(cur);
	fh1->BindToProcess(cur);
	fd[0]=fh0->GetFD();
	fd[1]=fh1->GetFD();
	VirtualMemorySpace::DisableAccessUser();
	return 0;
}

inline int Syscall_dup(int fd)
{
	Process *cur=POS_PM.Current();
	FileHandle *fh=cur->GetFileHandleFromFD(fd);
	if (fh==nullptr)
		return -1;
	FileHandle *nfh=fh->Dump();
	if (nfh==nullptr)
		return -1;
	nfh->BindToProcess(cur);
	return nfh->GetFD();
}

inline int Syscall_dup3(int fd,int nfd)
{
	if (fd==nfd)
		return fd;
	Process *cur=POS_PM.Current();
	FileHandle *fh=cur->GetFileHandleFromFD(fd);
	if (fh==nullptr)
		return -1;
	FileHandle *nfh=fh->Dump();
	FileHandle *ofh=cur->GetFileHandleFromFD(nfd);
	if (ofh!=nullptr)
		delete ofh;
	if (nfh==nullptr)
		return -1;
	if (nfh->BindToProcess(cur,nfd)!=ERR_None)
	{
		delete nfh;
		return -1;
	}
	return nfh->GetFD();
}

inline int Syscall_chdir(char *path)
{
	if (path==nullptr)
		return -1;
	VirtualMemorySpace::EnableAccessUser();
	POS_PM.Current()->SetCWD(path);
	VirtualMemorySpace::DisableAccessUser();
	return 0;
}

inline char* CurrentPathFromFileNameAndFD(int fd,char *filename)
{
	Process *cur=POS_PM.Current();
	char *path=nullptr;
	constexpr int AT_FDCWD=-100;
	if (VFSM.IsAbsolutePath(filename))
		path=VFSM.NormalizePath(filename);
	else if (fd==AT_FDCWD)
		path=VFSM.NormalizePath(filename,cur->GetCWD());
	else
	{
		FileHandle *fh=cur->GetFileHandleFromFD(fd);
		if (fh!=nullptr)
		{
			char *base=fh->Node()->GetPath<0>();
			if (base!=nullptr)
			{
				path=VFSM.NormalizePath(filename,base);
				Kfree(base);
			}
		}
	}
	return path;
}

inline int Syscall_openat(int fd,char *filename,int flags,int mode)//Currently, mode will be ignored...
{
	VirtualMemorySpace::EnableAccessUser();
	char *path=CurrentPathFromFileNameAndFD(fd,filename);
	VirtualMemorySpace::DisableAccessUser();
	if (path==nullptr)
		return -1;
	
	constexpr int O_CREAT=0x40,
				  O_RDONLY=0x000,
				  O_WRONLY=0x001,
				  O_RDWR=0x002,
				  O_DIRECTORY=0x0200000;//??
	FileNode *node=nullptr;
	if (flags&O_CREAT)
		if (flags&O_DIRECTORY)
			VFSM.CreateDirectory(path);
		else VFSM.CreateFile(path);
	node=VFSM.Open(path);
	if (node!=nullptr)
		if (node->IsDir()&&!(flags&O_DIRECTORY)||!node->IsDir()&&(flags&O_DIRECTORY))
			node=nullptr;
	FileHandle *re=nullptr;
	if (node!=nullptr)
	{
		Uint64 fh_flags=FileHandle::F_Seek|FileHandle::F_Size;//??
		if (flags&O_RDWR)
			fh_flags|=FileHandle::F_Read|FileHandle::F_Write;
		else if (flags&O_WRONLY)
			fh_flags|=FileHandle::F_Write;
		else fh_flags|=FileHandle::F_Read;
		re=new FileHandle(node,fh_flags);
		re->BindToProcess(POS_PM.Current());
	}
	Kfree(path);
	return re?re->GetFD():-1;
}

inline int Syscall_close(int fd)
{
	Process *cur=POS_PM.Current();
	FileHandle *fh=cur->GetFileHandleFromFD(fd);
	if (fh==nullptr)
		return -1;
	delete fh;//??
	return 0;
}

inline Sint64 Syscall_getdents64(int fd,RegisterData _buf,Uint64 bufSize)
{
	struct dirent
	{
		Uint64 ino;
		Sint64 off;
		Uint16 reclen;
		Uint8 name[0];
	}__attribute__((packed));
	char *dir=CurrentPathFromFileNameAndFD(fd,".");
	if (dir==nullptr)
		return -1;
	char *buf=(char*)_buf;
	char *childs[16];
	VirtualMemorySpace::EnableAccessUser();
	int skip=0,re=0;
	while (1)
	{
		int i=0,cnt=VFSM.GetAllFileIn(dir,childs,16,skip);
		while (i<cnt)
		{
			
			//...
			Kfree(childs[i]);
			++i;
		}
		if (cnt<16||i<cnt)
			break;
		else skip+=cnt;
	}
	VirtualMemorySpace::DisableAccessUser();
	//...
	
}

inline RegisterData Syscall_Read(int fd,void *dst,Uint64 size)
{
	FileHandle *fh=POS_PM.Current()->GetFileHandleFromFD(fd);
	if (fh==nullptr)
		return -1;
	VirtualMemorySpace::EnableAccessUser();
	Sint64 re=fh->Read(dst,size);
	VirtualMemorySpace::DisableAccessUser();
	return re<0?-1:re;
}

inline RegisterData Syscall_Write(int fd,void *src,Uint64 size)
{
	FileHandle *fh=POS_PM.Current()->GetFileHandleFromFD(fd);
	if (fh==nullptr)
		return -1;
	VirtualMemorySpace::EnableAccessUser();
	Sint64 re=fh->Write(src,size);
	VirtualMemorySpace::DisableAccessUser();
	return re<0?-1:re;
}

inline int Syscall_linkat(int olddirfd,char *oldpath,int newdirfd,char *newpath,unsigned flags)
{
	return -1;
}

inline int Syscall_unlinkat(int dirfd,char *path,unsigned flags)
{
	return -1;
}

inline int Syscall_mkdirat(int fd,char *filename,int mode)//Currently,mode will be ignored...
{
	VirtualMemorySpace::EnableAccessUser();
	char *path=CurrentPathFromFileNameAndFD(fd,filename);
	VirtualMemorySpace::DisableAccessUser();
	if (path==nullptr)
		return -1;
	ErrorType err=VFSM.CreateDirectory(path);
	Kfree(path);
	return InThisSet(err,ERR_None,ERR_FileAlreadyExist)/*??*/?0:-1;
}

inline int Syscall_unmount(const char *special,unsigned flags)
{
	kout[Warning]<<"Syscall_unmount is not usable yet!"<<endl;
	return 0;
}

inline int Syscall_mount(const char *special,const char *dir,const char *fstype,unsigned flags,const void *data)
{
	kout[Warning]<<"Syscall_mount is not usable yet!"<<endl;
	return 0;
}

inline int Syscall_fstat(int fd,RegisterData _kst)
{
	struct kstat
	{
		Uint64 st_dev;
		Uint64 st_ino;
		Uint32 st_mode;
		Uint32 st_nlink;
		Uint32 st_uid;
		Uint32 st_gid;
		Uint64 st_rdev;
		unsigned long __pad;
		int st_size;
		Uint32 st_blksize;
		int __pad2;
		Uint64 st_blocks;
		long st_atime_sec;
		long st_atime_nsec;
		long st_mtime_sec;
		long st_mtime_nsec;
		long st_ctime_sec;
		long st_ctime_nsec;
		unsigned __unused[2];
	}*kst=(kstat*)_kst;
	FileHandle *fh=POS_PM.Current()->GetFileHandleFromFD(fd);
	if (fh==nullptr)
		return -1;
	FileNode *node=fh->Node();
	if (node==nullptr)
		return -1;
	VirtualMemorySpace::EnableAccessUser();
	MemsetT<char>((char*)kst,0,sizeof(kstat));
	kst->st_size=node->Size();
	//<<Other info...
	VirtualMemorySpace::DisableAccessUser();
	return 0;
}

inline PID Syscall_Clone(TrapFrame *tf,Uint64 flags,void *stack,PID ppid,Uint64 tls,PID cid)
{
	if (ppid!=0||tls!=0||cid!=0)
		kout[Warning]<<"Syscall_Clone: Currently not support ppid,tls,cid parameter!"<<endl;
	constexpr Uint64 SIGCHLD=17;
	PID re=-1;
	ISAS
	{
		Process *cur=POS_PM.Current();
		Process *nproc=POS_PM.AllocProcess();
		ASSERTEX(nproc,"Syscall_Clone: Failed to create process!");
		nproc->Init(flags&SIGCHLD?0:Process::F_AutoDestroy);
		re=nproc->GetPID();
		nproc->SetStack(nullptr,cur->StackSize);
		if (stack==nullptr)//Aka fork
		{
			VirtualMemorySpace *nvms=KmallocT<VirtualMemorySpace>();
			nvms->Init();
			nvms->CreateFrom(cur->GetVMS());
			nproc->SetVMS(nvms);
			nproc->Start(tf,0);
		}
		else//Aka create thread 
		{
			nproc->SetVMS(cur->GetVMS());
			TrapFrame *ntf=(TrapFrame*)(nproc->Stack+nproc->StackSize)-1;
			MemcpyT<RegisterData>((RegisterData*)ntf,(const RegisterData*)tf,sizeof(TrapFrame)/sizeof(RegisterData));
			ntf->epc+=4;
			ntf->reg.sp=(RegisterData)stack;
			nproc->Start(ntf,1);
		}
		nproc->CopyOthers(cur);
		if (flags&SIGCHLD)
			nproc->SetFa(cur);
		nproc->stat=Process::S_Ready;
	}
	return re;
}

inline int Syscall_execve(const char *filepath,char *argvs[],char *envp[])//Currently, argvs and envps will be ingnored.
{
	Process *cur=POS_PM.Current(),*cp=nullptr;
	FileNode *node=VFSM.Open(cur,filepath);
	if (node==nullptr)
		return -1;
	FileHandle *file=new FileHandle(node);
	PID id=CreateProcessFromELF(file,0,cur->GetCWD());//PID will changed! Need improve...
	int re;
	delete file;
	if (id<=0)
		return -1;
	else
	{
		while ((cp=cur->GetQuitingChild(id))==nullptr)
			cur->GetWaitSem()->Wait();
		re=cp->GetReturnedValue();
		cp->Destroy();
	}
	cur->Exit(re);
	POS_PM.Schedule();
	kout[Fault]<<"Syscall_execve reached unreacheable branch!"<<endl;
}

inline PID Syscall_Wait4(PID cid,int *status,int options)
{
	constexpr int WNOHANG=1;
	Process *proc=POS_PM.Current();
	while (1)//??
	{
		Process *child=proc->GetQuitingChild(cid==-1?Process::AnyPID:cid);
		if (child!=nullptr)
		{
			PID re=child->GetPID();
			if (status!=nullptr)
			{
				VirtualMemorySpace::EnableAccessUser();
				*status=child->GetReturnedValue()<<8;//??
				VirtualMemorySpace::DisableAccessUser();
			}
			child->Destroy();
			return re;
		}
		else if (options&WNOHANG)
			return -1;
		else proc->GetWaitSem()->Wait();
	}
}

inline PID Syscall_GetPPID()
{
	Process *proc=POS_PM.Current();
	Process *fa=proc->GetFa();
	if (fa==nullptr)
		return Process::InvalidPID;
	else return fa->GetPID();
}

inline PID Syscall_GetPID()
{return POS_PM.Current()->GetPID();}

inline PtrInt Syscall_brk(PtrInt pos)
{
	HeapMemoryRegion *hmr=POS_PM.Current()->GetHeap();
	if (hmr==nullptr)
		return -1;
	if (pos==0)
		return hmr->BreakPoint();
	ErrorType err=hmr->Resize(pos-hmr->BreakPoint());
	if (err==ERR_None)
		return 0;
	else return -1;	
}

inline int Syscall_munmap(void *start,Uint64 len)
{
	VirtualMemorySpace *vms=POS_PM.Current()->GetVMS();
	VirtualMemoryRegion *vmr=vms->FindVMR((PtrInt)start);
	if (vmr==nullptr)
		-1;
	MemapFileRegion *mfr=(MemapFileRegion*)vmr;
	ErrorType err=mfr->Save();
	if (err)
		kout[Error]<<"Syscall_munmap: mfr failed to save! ErrorCode: "<<err<<endl;
	delete mfr;
	return 0;
}

inline PtrInt Syscall_mmap(void *start,Uint64 len,int prot,int flags,int fd,int off)//Currently flags will be ignored...
{
	FileHandle *fh=POS_PM.Current()->GetFileHandleFromFD(fd);
	if (fh==nullptr)
		return -1;
	FileNode *node=fh->Node();
	if (node==nullptr)
		return -1;
	constexpr Uint64 PROT_NONE=0,
					 PROT_READ=1,
					 PROT_WRITE=2,
					 PROT_EXEC=4,
					 PROT_GROWSDOWN=0X01000000,//Unsupported yet...
					 PROT_GROWSUP=0X02000000;//Unsupported yet...
	Uint64 mfrProt=VirtualMemoryRegion::VM_File;
	if (prot&PROT_READ)
		mfrProt|=VirtualMemoryRegion::VM_Read;
	if (prot&PROT_WRITE)
		mfrProt|=VirtualMemoryRegion::VM_Write;
	if (prot&PROT_EXEC)
		mfrProt|=VirtualMemoryRegion::VM_Exec;
	PtrInt s=POS_PM.Current()->GetVMS()->GetUsableVMR(start==nullptr?0x60000000:(PtrInt)start,(PtrInt)0x80000000/*??*/,len);
	if (s==0||start!=nullptr&&((PtrInt)start>>PageSizeBit<<PageSizeBit)!=s)
		return -1;
	MemapFileRegion *mfr=new MemapFileRegion(node,start==nullptr?(void*)s:start,len,off,mfrProt);
	if (mfr==nullptr)
		return -1;
	POS_PM.Current()->GetVMS()->InsertVMR(mfr);
	ErrorType err=mfr->Load();
	if (err)
		kout[Error]<<"Syscall_mmap: mfr failed to load! ErrorCode: "<<err<<endl;
	return start==nullptr?s:(PtrInt)start;
}

inline RegisterData Syscall_times(RegisterData _tms)
{
	struct TMS              
	{                     
		long tms_utime;  
		long tms_stime;  
		long tms_cutime; 
		long tms_cstime; 
	}*tms=(TMS*)_tms;
	ClockTime unit=Timer_1us;//??
	if (tms!=nullptr)
	{
		Process *cur=POS_PM.Current();
		ClockTime rd=cur->GetRunningDuration(1);
//		ClockTime st=cur->GetStartedTime(0);
//		ClockTime sd=cur->GetSleepingDuration(0);
//		ClockTime wd=cur->GetWaitingDuration(0);
		ClockTime ud=cur->GetUserDuration(0);
		cur->GetVMS()->EnableAccessUser();
		tms->tms_utime=ud/unit;
		tms->tms_stime=(rd-ud)/unit;
		tms->tms_cutime=0;//??
		tms->tms_stime=0;
		cur->GetVMS()->DisableAccessUser();
	}
	return GetClockTime();
}

inline RegisterData Syscall_Uname(RegisterData p)
{
	struct utsname
	{
		char sysname[65];
		char nodename[65];
		char release[65];
		char version[65];
		char machine[65];
		char domainname[65];
	}*u=(utsname*)p;
	Process *proc=POS_PM.Current();
	VirtualMemorySpace *vms=proc->GetVMS();
	vms->EnableAccessUser();
	strCopy(u->sysname,"PAL_OperatingSystem");
	strCopy(u->nodename,"PAL_OperatingSystem");
	strCopy(u->release,"Debug");
	strCopy(u->version,"0.3");
	strCopy(u->machine,"Riscv64");
	strCopy(u->domainname,"PAL");
	vms->DisableAccessUser();
	return 0;
}

inline RegisterData Syscall_sched_yeild()
{
	Process *proc=POS_PM.Current();
	proc->Rest();
	return 0;
}

inline RegisterData Syscall_gettimeofday(RegisterData _tv)
{
	struct timeval
	{
		Uint64 tv_sec;
		Uint64 tv_usec;
	}*tv=(timeval*)_tv;
	//<<Improve timer related to 1970...
	VirtualMemorySpace::EnableAccessUser();
	ClockTime t=GetClockTime();
	tv->tv_sec=t/Timer_1s;
	tv->tv_usec=t%Timer_1s/Timer_1us;
	VirtualMemorySpace::DisableAccessUser();
	return 0;
}

inline RegisterData Syscall_nanosleep(RegisterData _req,RegisterData _rem)
{
	struct timespec
	{
		int tv_sec;
		int tv_nsec;
	}
	*req=(timespec*)_req,
	*rem=(timespec*)_rem;
	VirtualMemorySpace::EnableAccessUser();
	Semaphore sem(0);
	sem.Wait(req->tv_sec*Timer_1s+
			 req->tv_nsec/1000000*Timer_1ms+
			 req->tv_nsec%1000000/1000*Timer_1us+
			 req->tv_nsec%1000*Timer_1ns);
	rem->tv_sec=rem->tv_nsec=0;//??
	VirtualMemorySpace::DisableAccessUser();
	return 0;
}

ErrorType TrapFunc_Syscall(TrapFrame *tf)
{
	InterruptStackAutoSaverBlockController isas;//??
//	kout[Test]<<"Syscall "<<tf->reg.a7<<" | "<<(void*)tf->reg.a0<<" "<<(void*)tf->reg.a1<<" "<<(void*)tf->reg.a2<<" "<<(void*)tf->reg.a3<<" "<<(void*)tf->reg.a4<<" "<<(void*)tf->reg.a5<<endl;
	switch (tf->reg.a7)
	{
		case SYS_Putchar:
			Syscall_Putchar(tf->reg.a0);
			break;
		case SYS_Getchar:
			tf->reg.a0=Syscall_Getchar();
			break;
		case SYS_Getputchar:
			tf->reg.a0=Syscall_Getputchar();
			break;
		case SYS_Exit:
			Syscall_Exit(tf->reg.a0);
			break;
		case SYS_Fork:
			Syscall_Fork(tf);
			break;
		case SYS_GetPID:
			tf->reg.a0=Syscall_GetPID();
			break;
		
		case	SYS_getcwd		:
			tf->reg.a0=(RegisterData)Syscall_getcwd((char*)tf->reg.a0,tf->reg.a1);
			break;
		case	SYS_pipe2		:
			tf->reg.a0=Syscall_pipe2((int*)tf->reg.a0,tf->reg.a1);
			break;
		case	SYS_dup			:
			tf->reg.a0=Syscall_dup(tf->reg.a0);
			break;
		case	SYS_dup3		:
			tf->reg.a0=Syscall_dup3(tf->reg.a0,tf->reg.a1);
			break;
		case	SYS_chdir		:
			tf->reg.a0=Syscall_chdir((char*)tf->reg.a0);
			break;
		case	SYS_openat		:
			tf->reg.a0=Syscall_openat(tf->reg.a0,(char*)tf->reg.a1,tf->reg.a2,tf->reg.a3);
			break;
		case	SYS_close		:
			tf->reg.a0=Syscall_close(tf->reg.a0);
			break;
		case	SYS_getdents64	:
			goto Default;
		case	SYS_read		:
			tf->reg.a0=Syscall_Read(tf->reg.a0,(void*)tf->reg.a1,tf->reg.a2);
			break;
		case	SYS_write		:
			tf->reg.a0=Syscall_Write(tf->reg.a0,(void*)tf->reg.a1,tf->reg.a2);
			break;
		case	SYS_linkat		:
			tf->reg.a0=Syscall_linkat(tf->reg.a0,(char*)tf->reg.a1,tf->reg.a2,(char*)tf->reg.a3,tf->reg.a4);
			break;
		case	SYS_unlinkat	:
			tf->reg.a0=Syscall_unlinkat(tf->reg.a0,(char*)tf->reg.a1,tf->reg.a2);
			break;
		case	SYS_mkdirat		:
			tf->reg.a0=Syscall_mkdirat(tf->reg.a0,(char*)tf->reg.a1,tf->reg.a2);
			break;
		case	SYS_umount2		:
			tf->reg.a0=Syscall_unmount((const char*)tf->reg.a0,tf->reg.a1);
			break;
		case	SYS_mount		:
			tf->reg.a0=Syscall_mount((const char*)tf->reg.a0,(const char*)tf->reg.a1,(const char*)tf->reg.a2,tf->reg.a3,(void*)tf->reg.a4);
			break;
		case	SYS_fstat		:
			tf->reg.a0=Syscall_fstat(tf->reg.a0,tf->reg.a1);
			break;
		case	SYS_clone		:
			tf->reg.a0=Syscall_Clone(tf,tf->reg.a0,(void*)tf->reg.a1,tf->reg.a2,tf->reg.a3,tf->reg.a4);
			break;
		case	SYS_execve		:
			Syscall_execve((char*)tf->reg.a0,(char**)tf->reg.a1,(char**)tf->reg.a2);
			break;
		case	SYS_wait4		:
			tf->reg.a0=Syscall_Wait4(tf->reg.a0,(int*)tf->reg.a1,tf->reg.a2);
			break;
		case	SYS_exit		:
			Syscall_Exit(tf->reg.a0);
			break;
		case	SYS_getppid		:
			tf->reg.a0=Syscall_GetPPID();
			break;
		case	SYS_getpid		:
			tf->reg.a0=Syscall_GetPID();
			break;
		case	SYS_brk			:
			tf->reg.a0=Syscall_brk(tf->reg.a0);
			break;
		case	SYS_munmap		:
			tf->reg.a0=Syscall_munmap((void*)tf->reg.a0,tf->reg.a1);
			break; 
		case	SYS_mmap		:
			tf->reg.a0=Syscall_mmap((void*)tf->reg.a0,tf->reg.a1,tf->reg.a2,tf->reg.a3,tf->reg.a4,tf->reg.a5);
			break;
		case	SYS_times		:
			tf->reg.a0=Syscall_times(tf->reg.a0);
			break;
		case	SYS_uname		:
			tf->reg.a0=Syscall_Uname(tf->reg.a0);
			break;
		case	SYS_sched_yeild	:
			tf->reg.a0=Syscall_sched_yeild();
			break;
		case	SYS_gettimeofday:
			tf->reg.a0=Syscall_gettimeofday(tf->reg.a0);
			break;
		case	SYS_nanosleep	:
			tf->reg.a0=Syscall_nanosleep(tf->reg.a0,tf->reg.a1);
			break;
		default:
		Default:
		{
			Process *cur=POS_PM.Current();
			if (cur->IsKernelProcess())
				kout[Fault]<<"TrapFunc_Syscall: Unknown syscall "<<tf->reg.a7<<" from kernel process "<<cur->GetPID()<<"!"<<endl;
			else
			{
				kout[Error]<<"TrapFunc_Syscall: Unknown syscall "<<tf->reg.a7<<" from user process "<<cur->GetPID()<<"!"<<endl;
				cur->Exit(Process::Exit_BadSyscall);
				POS_PM.Schedule();
				kout[Fault]<<"TrapFunc_Syscall: Reaced unreachable branch!"<<endl;
			}
			break;
		}
	}
	tf->epc+=4;
	return ERR_None;
}
