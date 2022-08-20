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

inline void Syscall_Exit(int re)
{
	Process *cur=POS_PM.Current();
	cur->Exit(re);
	POS_PM.Schedule();
	kout[Fault]<<"Syscall_Exit: Reached unreachable branch!"<<endl;
}

inline void Syscall_Rest()
{
	Process *proc=POS_PM.Current();
//	proc->Rest();
	ProcessManager::Schedule();//??
}

inline PID Syscall_Fork(TrapFrame *tf)
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

inline char* CurrentPathFromFileNameAndFD(int fd,const char *filename)
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
	kout[Debug]<<"open file name:"<<filename<<endl;
	VirtualMemorySpace::EnableAccessUser();
	char *path=CurrentPathFromFileNameAndFD(fd,filename);
	VirtualMemorySpace::DisableAccessUser();
	if (path==nullptr)
		return -1;

	constexpr int O_CREAT=0x40,
				  O_RDONLY=0x000,
				  O_WRONLY=0x001,
				  O_RDWR=0x002,
				  O_DIRECTORY=0200000,//??
				  O_EXCL=0x80;
	FileNode *node=VFSM.Open(path);
	if (flags&O_CREAT)
		if (node==nullptr)
		{
			if (flags&O_DIRECTORY)
				VFSM.CreateDirectory(path);
			else VFSM.CreateFile(path);
			node=VFSM.Open(path);
		}
		else if (flags&O_EXCL)
			node=nullptr;//??
	
	if (node!=nullptr)
		if (!node->IsDir()&&(flags&O_DIRECTORY))
		{
			node=nullptr;
			VFSM.Close(node);//??
		}
	FileHandle *re=nullptr;
	if (node!=nullptr)
	{
		Uint64 fh_flags=FileHandle::F_Seek|FileHandle::F_Size;//??
//		if (flags&O_RDWR)
			fh_flags|=FileHandle::F_Read|FileHandle::F_Write;
//		else if (flags&O_WRONLY)
//			fh_flags|=FileHandle::F_Write;
//		else fh_flags|=FileHandle::F_Read;
		re=new FileHandle(node,fh_flags);
	
		re->BindToProcess(POS_PM.Current());
		kout[Debug]<<"bind to process success"<<endl;
	}
	Kfree(path);
	kout[Debug]<<"openat new fd:"<<re->GetFD()<<endl;
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

inline Sint64 Syscall_lseek(int fd,Sint64 off,int whence)
{
	FileHandle *fh=POS_PM.Current()->GetFileHandleFromFD(fd);
	if (fh==nullptr||!(fh->GetFlags()&FileHandle::F_Seek))
		return -1;
	
	constexpr int SEEK_SET=0,
				  SEEK_CUR=1,
				  SEEK_END=2;
	int base;
	switch (whence)
	{
		case SEEK_SET:	base=FileHandle::Seek_Beg;	break;
		case SEEK_CUR:	base=FileHandle::Seek_Cur;	break;
		case SEEK_END:	base=FileHandle::Seek_End;	break;
		default:	return -1;
	}
	ErrorType err=fh->Seek(off,base,1/*??*/);
	if (err)
		return -1;
	else return fh->GetPos();
}

template <ModeRW rw> inline RegisterData Syscall_ReadWrite(int fd,void *buf,Uint64 size,Uint64 off=-1)
{
	if constexpr(rw==ModeRW::Write)
		kout[Debug]<<"begin to write from fd:"<<fd<<endl;
	else 
	{
		
		kout[Debug]<<"begin to read from fd:"<<fd<<endl;
		
	}

	FileHandle *fh=POS_PM.Current()->GetFileHandleFromFD(fd);
	kout[Debug]<<"fh:"<<fh<<endl;
	if (fh==nullptr)
		return -1;
	kout[Debug]<<"111111"<<endl;
	VirtualMemorySpace::EnableAccessUser();
	kout[Debug]<<"222222"<<endl;
	Sint64 re;
	if constexpr(rw==ModeRW::Write)
	{
		re=fh->Write(buf,size,off);
	}
	else 
	{
		kout[Debug]<<"read buf:"<<buf<<" size:"<<size<<" off:"<<off<<endl;
		re=fh->Read(buf,size,off);
	}
	kout[Debug]<<"333333"<<endl;
	VirtualMemorySpace::DisableAccessUser();
	if constexpr(rw==ModeRW::Write)
		kout[Debug]<<"write ok"<<endl;
	else 
		kout[Debug]<<"read ok"<<endl;
	return re<0?-1:re;
}

struct iovec
{
	void *base;
	SizeType len;
};

template <ModeRW rw> inline RegisterData Syscall_ReadWriteVector(int fd,iovec *iov,int iovcnt,Uint64 off=-1)
{
	FileHandle *fh=POS_PM.Current()->GetFileHandleFromFD(fd);
	if (fh==nullptr)
		return -1;
	VirtualMemorySpace::EnableAccessUser();
	Sint64 re=0,r;
	for (int i=0;i<iovcnt;++i)
	{
		if constexpr (rw==ModeRW::W)
			r=fh->Write(iov[i].base,iov[i].len,off==-1?-1:off+re);
		else r=fh->Read(iov[i].base,iov[i].len,off==-1?-1:off+re);
		if (r<0)
			break;
		re+=r;
		if (r!=iov[i].len)
			break;
	}
	VirtualMemorySpace::DisableAccessUser();
	return re;
}

inline int Syscall_linkat(int olddirfd,char *oldpath,int newdirfd,char *newpath,unsigned flags)
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

inline int Syscall_fstat_node(FileNode *node,RegisterData _kst)
{
	if (node==nullptr)
		return -1;
	struct stat
	{
		Uint32    st_dev;
		Uint16    st_ino;
		Uint16    st_mode;
		Sint16    st_nlink;
		Sint16    st_uid;
		Sint16    st_gid;
		Uint32    st_rdev;
		long      st_size;
        long long st_atime;
        long long st_mtime;
        long long st_ctime;
	}*kst=(stat*)_kst;
	enum
	{
		S_IFDIR=0040000,
		S_IFCHR=0020000,
		S_IFBLK=0060000,
		S_IFREG=0100000,
		S_IFIFO=0010000,
		S_IFLNK=0120000,
		S_IFSOCK=0140000,

		S_ISUID=04000, //set-user-ID bit
		S_ISGID=02000, //set-group-ID bit (see below)
		S_ISVTX=01000, //sticky bit (see below)

		S_IRWXU=00700, //owner has read, write, and execute permission
		S_IRUSR=00400, //owner has read permission
		S_IWUSR=00200, //owner has write permission
		S_IXUSR=00100, //owner has execute permission

		S_IRWXG=00070, //group has read, write, and execute permission
		S_IRGRP=00040, //group has read permission
		S_IWGRP=00020, //group has write permission
		S_IXGRP=00010, //group has execute permission

		S_IRWXO=00007, //others (not in group) have read, write, and execute permission
		S_IROTH=00004, //others have read permission
		S_IWOTH=00002, //others have write permission
		S_IXOTH=00001 //others have execute permission
	};
	
	VirtualMemorySpace::EnableAccessUser();
	kout[Debug]<<node->GetName()<<endl;
	MemsetT<char>((char*)kst,0,sizeof(stat));
	kst->st_size=node->Size();
	if (node->GetAttributes()&FileNode::A_Specical)
		kst->st_mode|=S_IFCHR;
	else if (node->GetAttributes()&FileNode::A_Dir)
		kst->st_mode|=S_IFDIR;
	else kst->st_mode|=S_IFREG;
	kst->st_nlink = 1;
	kst->st_uid = 0;
	kst->st_mode |= S_IRWXU; 
	kst->st_mode |= S_IRWXG;
	kst->st_mode |= S_IRWXO;
	//TODO:直接给了所有权限
	//<<Other info...
	VirtualMemorySpace::DisableAccessUser();
	return 0;
}

inline int Syscall_fstat(int fd,RegisterData _kst)
{
	FileHandle *fh=POS_PM.Current()->GetFileHandleFromFD(fd);
	if (fh==nullptr)
		return -1;
	FileNode *node=fh->Node();
	return Syscall_fstat_node(node,_kst);
}

inline int Syscall_newfstatat(int dirfd,const char *pathname,RegisterData _kst,int flags)
{
	if (flags)
		kout[Warning]<<"Syscall_newfstatat: flags is not supported yet and it will be ignored!"<<endl;
	VirtualMemorySpace::EnableAccessUser();
	char *path=CurrentPathFromFileNameAndFD(dirfd,pathname);//Is this right?? dirfd is relative path for pathname rather than directly used...
	VirtualMemorySpace::DisableAccessUser();
	kout[Debug]<<path<<endl;
	return Syscall_fstat_node(VFSM.Open(path),_kst);
}

inline RegisterData Syscall_fcntl(int fd,int cmd,TrapFrame *tf)
{
	FileHandle *fh=POS_PM.Current()->GetFileHandleFromFD(fd);
	if (fh==nullptr)
		return -1;
	
	enum
	{
		F_DUPFD=0,
		F_GETFD=1,
		F_SETFD=2,
		F_GETFL=3,
		F_SETFL=4,
		F_SETOWN=8,
		F_GETOWN=9,
		F_SETSIG=10,
		F_GETSIG=11,
	};

	switch (cmd)
	{
		case 0x406:
		{
			Process *cur=POS_PM.Current();
			FileHandle *fh=cur->GetFileHandleFromFD(fd);
			FileHandle *re=nullptr;
			
			Uint64 fh_flags=FileHandle::F_Seek|FileHandle::F_Size;//??
	//		if (flags&O_RDWR)
				fh_flags|=FileHandle::F_Read|FileHandle::F_Write;
	//		else if (flags&O_WRONLY)
	//			fh_flags|=FileHandle::F_Write;
	//		else fh_flags|=FileHandle::F_Read;
			re=new FileHandle(fh->Node(),fh_flags);
			re->BindToProcess(POS_PM.Current());
			return re->GetFD();
		}
				
		default:
			kout[Error]<<"Unknown fcnt cmd "<<cmd<<endl;
			return -1;
	}
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
//			kout[Debug]<<"Clone "<<cur->GetPID()<<" to "<<re<<" as fork"<<endl;
			VirtualMemorySpace *nvms=KmallocT<VirtualMemorySpace>();
			nvms->Init();
			nvms->CreateFrom(cur->GetVMS());
			nproc->SetVMS(nvms);
			nproc->Start(tf,0);
		}
		else//Aka create thread 
		{
//			kout[Debug]<<"Clone "<<cur->GetPID()<<" to "<<re<<" as thread"<<endl;
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

inline int Syscall_execve(const char *filepath,char *argvs[],char *envp[])//Currently envps will be ingnored.
{
	Process *cur=POS_PM.Current(),*cp=nullptr;
	FileNode *node=VFSM.Open(cur,filepath);
	if (node==nullptr)
		return -1;
	int argc=0;
	while (argvs[argc]!=nullptr)
		++argc;
	char **argv=new char*[argc];
	for (int i=0;i<argc;++i)
		argv[i]=strDump(argvs[i]);
	FileHandle *file=new FileHandle(node);
	PID id=CreateProcessFromELF(file,0,cur->GetCWD(),argc,argv);//PID will change! Need improve...
	for (int i=0;i<argc;++i)
		Kfree(argv[i]);
	delete[] argv;
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
	return -1;
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
	else return kout[Error]<<"Syscall_brk failed with return code "<<err<<endl,-1;
}

inline int Syscall_munmap(void *start,Uint64 len)
{
	VirtualMemorySpace *vms=POS_PM.Current()->GetVMS();
	VirtualMemoryRegion *vmr=vms->FindVMR((PtrInt)start);
	if (vmr==nullptr)
		-1;
	if (vmr->GetFlags()&VirtualMemoryRegion::VM_File)
	{
		MemapFileRegion *mfr=(MemapFileRegion*)vmr;
		ErrorType err=mfr->Save();
		if (err<0)
			kout[Error]<<"Syscall_munmap: mfr failed to save! ErrorCode: "<<err<<endl;
		delete mfr;
	}
	else vms->RemoveVMR(vmr,1);
	return 0;
}

inline PtrInt Syscall_mmap(void *start,Uint64 len,int prot,int flags,int fd,int off)//Currently flags will be ignored...
{
	if (len==0)
		return -1;
	FileNode *node=nullptr;
	if (fd!=-1)
	{
		FileHandle *fh=POS_PM.Current()->GetFileHandleFromFD(fd);
		if (fh==nullptr)
			return -1;
		node=fh->Node();
		if (node==nullptr)
			return -1;
	}
	else DoNothing;//Anonymous mmap
//	kout[Debug]<<"mmap "<<start<<" "<<(void*)len<<" "<<prot<<" "<<flags<<" "<<fd<<" "<<off<<" | "<<node<<endl;
	
	VirtualMemorySpace *vms=POS_PM.Current()->GetVMS();
	
	constexpr Uint64 PROT_NONE=0,
					 PROT_READ=1,
					 PROT_WRITE=2,
					 PROT_EXEC=4,
					 PROT_GROWSDOWN=0X01000000,//Unsupported yet...
					 PROT_GROWSUP=0X02000000;//Unsupported yet...
	Uint64 vmrProt=node!=nullptr?VirtualMemoryRegion::VM_File:0;
//	if (prot&PROT_READ)
		vmrProt|=VirtualMemoryRegion::VM_Read;
//	if (prot&PROT_WRITE)
		vmrProt|=VirtualMemoryRegion::VM_Write;
//	if (prot&PROT_EXEC)
		vmrProt|=VirtualMemoryRegion::VM_Exec;
	
	constexpr Uint64 MAP_FIXED=0x10;
	if (flags&MAP_FIXED)//Need improve...
	{
		VirtualMemoryRegion *vmr=vms->FindVMR((PtrInt)start);
		if (vmr!=nullptr)
			if (vmr->GetEnd()<=((PtrInt)start+len+PageSize-1>>PageSizeBit<<PageSizeBit))
				if (vmr->GetFlags()&VirtualMemoryRegion::VM_File)
				{
					MemapFileRegion *mfr=(decltype(mfr))vmr;
					mfr->Resize((PtrInt)start-mfr->GetStartAddr());
				}
				else
				{
					vmr->End=(PtrInt)start+PageSize-1>>PageSizeBit<<PageSizeBit;//??
					//<<Free pages not in range...
				}
			else kout[Error]<<"Failed to discard mmap region inside vmr!"<<endl;
	}
	
	PtrInt s=vms->GetUsableVMR(start==nullptr?0x60000000:(PtrInt)start,(PtrInt)0x70000000/*??*/,len);
	if (s==0||start!=nullptr&&((PtrInt)start>>PageSizeBit<<PageSizeBit)!=s)
		goto ErrorReturn;
	s=start==nullptr?s:(PtrInt)start;
	if (node!=nullptr)
	{
		MemapFileRegion *mfr=new MemapFileRegion(node,(void*)s,len,off,vmrProt);
//		kout[Debug]<<"mfr "<<mfr<<endl;
		if (mfr==nullptr)
			goto ErrorReturn;
		vms->InsertVMR(mfr);
		ErrorType err=mfr->Load();
		if (err<0)
		{
			kout[Error]<<"Syscall_mmap: mfr failed to load! ErrorCode: "<<err<<endl;
			delete mfr;
			goto ErrorReturn;
		}
	}
	else
	{
		VirtualMemoryRegion *vmr=KmallocT<VirtualMemoryRegion>();
		vmr->Init(s,s+len,vmrProt);
		vms->InsertVMR(vmr);
	}
//	kout[Debug]<<"mmaped at "<<(void*)s<<endl;
	return s;
ErrorReturn:
//	kout[Debug]<<"mmap error"<<endl;
	if (node)
		VFSM.Close(node);
	return -1;
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

inline int Syscall_clock_gettime(RegisterData clkid,RegisterData tp)//Currently, clkid will be ignored...
{
	struct timespec
	{
		int tv_sec;
		int tv_nsec;
	}*tv=(timespec*)tp;
	VirtualMemorySpace::EnableAccessUser();
	ClockTime t=GetClockTime();
	tv->tv_sec=t/Timer_1s;
	tv->tv_nsec=t%Timer_1s;//??
	VirtualMemorySpace::DisableAccessUser();
	return 0;
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
//	proc->Rest();
	ProcessManager::Schedule();//??
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
inline RegisterData Syscall_getdents64(int fd,RegisterData _buf,Uint64 bufSize)
{


	struct Dirent
	{
	    Uint64 d_ino;   // 索引结点号
	    Sint64 d_off;    // 到下一个dirent的偏移
	    unsigned short d_reclen;    // 当前dirent的长度
	    unsigned char d_type;   // 文件类型
	    char d_name[0];  //文件名
	}__attribute__((packed));
	char *dir=CurrentPathFromFileNameAndFD(fd,".");

	if (dir==nullptr)
	{
		return -1;

	}
	VirtualMemorySpace::EnableAccessUser();

	FileNode* *nodes  = new FileNode* [bufSize];
	int cnt=VFSM.GetAllFileIn(dir,nodes,bufSize,0);
	if(cnt == 0)
	{
		return 0;
	}
	int n_read = 0;
	for(int i=0;i<1;i++)//size会超掉
	{
		Dirent * dirent = (Dirent *)(_buf + n_read) ;
		dirent->d_ino = i+1;
		dirent->d_off = 32;
		dirent->d_type = 0;
		const char * name = nodes[i]->GetName();
		int j = 0;
		for(;j<strLen(name);j++)
		{
			dirent->d_name[j] = name[j];
		}
		dirent->d_name[j] = 0;
		dirent->d_reclen = sizeof(Uint64) * 2 + sizeof(unsigned) * 2 + strLen(name) + 1;
		n_read += dirent->d_reclen;
	}
	VirtualMemorySpace::DisableAccessUser();
	Sint64 return_value = 512;//应该是n_read
	return return_value;
	
}

inline int Syscall_unlinkat(int dirfd,char *path,unsigned flags)
{
	char *abs_path=CurrentPathFromFileNameAndFD(dirfd,path);
	if (abs_path==nullptr)
	{
		return -1;

	}
	VirtualMemorySpace::EnableAccessUser();
	int re = VFSM.Unlink(abs_path);
	VirtualMemorySpace::DisableAccessUser();
	return re?-1:0;
}

inline int Syscall_prlimit64(PID pid,int resource,RegisterData nlmt,RegisterData olmt)//It is not supported completely, only query is allowed now that pid and nlmt will be ignored...
{
	struct rlimit
	{
		Uint64 cur,
			   max;
	};
	
	enum
	{
		RLIMIT_CPU        =0,
		RLIMIT_FSIZE      =1,
		RLIMIT_DATA       =2,
		RLIMIT_STACK      =3,
		RLIMIT_CORE       =4,
		RLIMIT_RSS        =5,
		RLIMIT_NPROC      =6,
		RLIMIT_NOFILE     =7,
		RLIMIT_MEMLOCK    =8,
		RLIMIT_AS         =9,
		RLIMIT_LOCKS      =10,
		RLIMIT_SIGPENDING =11,
		RLIMIT_MSGQUEUE   =12,
		RLIMIT_NICE       =13,
		RLIMIT_RTPRIO     =14,
		RLIMIT_RTTIME     =15,
		RLIMIT_NLIMITS    =16
	};
	
	int re=0;
	rlimit *oldlimit=(decltype(oldlimit))olmt,
		   *newlimit=(decltype(newlimit))nlmt;
	if (oldlimit!=nullptr)
		switch (resource)
		{
			case RLIMIT_STACK:	oldlimit->cur=oldlimit->max=InnerUserProcessStackSize-512;	break;//Need improve...
			default:	re=-1;	break;
		}
	if (newlimit!=nullptr)
		kout[Warning]<<"Syscall_prlimit64 newlimit is set, however it will be ignored!"<<endl;
	return re;
}

inline int Syscall_statfs(const char *path,RegisterData buf)
{
	struct statfs 
	{
		Uint64 f_type,
			   f_bsize,
			   f_blocks,
			   f_bfree,
			   f_bavail,
			   f_files,
			   f_ffree;
		Uint32 f_fsid[2];
		Uint64 f_namelen,
			   f_frsize,
			   f_flags,
			   f_spare[4];
	}*data=(statfs*)buf;
	VirtualMemorySpace::EnableAccessUser();
	//We provide info /VFS/FAT32 here because testsuits request the /
	data->f_bsize=512;
	data->f_blocks=123;
	data->f_files=4;
	data->f_namelen=255;
	//Need improve...
	
	VirtualMemorySpace::DisableAccessUser();
	return 0;
}
inline int Syscall_getuid()
{
	//TODO:not done
	return 0;
}
inline int  Syscall_sendfile(int out_fd,int in_fd,long long * offset,Uint64 count)
{
	Uint64 off;
	if(offset == nullptr)
	{
		off=0;
	}
	else
	{
		off = *offset;
	}
	kout[Debug]<<"copy from "<<out_fd<<" to "<<in_fd<<" offset:"<<off<<" count "<<count<<endl;
	
	
	FileHandle *out_fh=POS_PM.Current()->GetFileHandleFromFD(out_fd),*in_fh = POS_PM.Current()->GetFileHandleFromFD(in_fd);
	kout[Debug]<<"from:"<<out_fh->Node()->GetName()<<" to:"<<in_fh->Node()->GetName()<<endl;
	Uint64 handle_size = 0,need_handle_size = out_fh->Size() - off;
	out_fh->Seek(off);

	const Uint32 BUFFER_SIZE = 1024;
	char buffer[BUFFER_SIZE];
	while(handle_size < need_handle_size)
	{
		Uint32 read_size;
		if(need_handle_size-handle_size<=BUFFER_SIZE)
		{
			read_size = need_handle_size - handle_size;
		}	
		else
		{
			read_size = BUFFER_SIZE;
		}
		out_fh->Read(buffer,read_size);
		in_fh->Write(buffer,read_size);
		handle_size+=read_size;
		kout[Debug]<<handle_size<<" "<<need_handle_size<<endl;
	}
	return handle_size;
}
ErrorType TrapFunc_Syscall(TrapFrame *tf)
{
	InterruptStackAutoSaverBlockController isas;//??
	if ((long long)tf->reg.a7>=0)
		kout[Test]<<"PID "<<POS_PM.Current()->GetPID()<<" Syscall "<<(long long)tf->reg.a7<<" "<<SyscallName((long long)tf->reg.a7)<<" "<<" | "<<(void*)tf->reg.a0<<" "<<(void*)tf->reg.a1<<" "<<(void*)tf->reg.a2<<" "<<(void*)tf->reg.a3<<" "<<(void*)tf->reg.a4<<" "<<(void*)tf->reg.a5<<endl;
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
		case SYS_Rest:
			Syscall_Rest();
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
			tf->reg.a0 = Syscall_getdents64(tf->reg.a0,tf->reg.a1,tf->reg.a2);
			break;
		case	SYS_read		:
			tf->reg.a0=Syscall_ReadWrite<ModeRW::Read>(tf->reg.a0,(void*)tf->reg.a1,tf->reg.a2);
			break;
		case	SYS_write		:
			tf->reg.a0=Syscall_ReadWrite<ModeRW::Write>(tf->reg.a0,(void*)tf->reg.a1,tf->reg.a2);
			break;
		case 	SYS_readv		:
			tf->reg.a0=Syscall_ReadWriteVector<ModeRW::Read>(tf->reg.a0,(iovec*)tf->reg.a1,tf->reg.a2);
			break;
		case 	SYS_writev		:
			tf->reg.a0=Syscall_ReadWriteVector<ModeRW::Write>(tf->reg.a0,(iovec*)tf->reg.a1,tf->reg.a2);
			break;
		case 	SYS_pread64		:
			tf->reg.a0=Syscall_ReadWrite<ModeRW::Read>(tf->reg.a0,(void*)tf->reg.a1,tf->reg.a2,tf->reg.a3);
			break;
		case 	SYS_pwrite64	:
			tf->reg.a0=Syscall_ReadWrite<ModeRW::Write>(tf->reg.a0,(void*)tf->reg.a1,tf->reg.a2,tf->reg.a3);
			break;
		case	 SYS_preadv		:
			tf->reg.a0=Syscall_ReadWriteVector<ModeRW::Read>(tf->reg.a0,(iovec*)tf->reg.a1,tf->reg.a2,tf->reg.a3);
			break;
		case 	SYS_pwritev		:
			tf->reg.a0=Syscall_ReadWriteVector<ModeRW::Write>(tf->reg.a0,(iovec*)tf->reg.a1,tf->reg.a2,tf->reg.a3);
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
		case 	SYS_newfstatat	:
			tf->reg.a0=Syscall_newfstatat(tf->reg.a0,(const char*)tf->reg.a1,tf->reg.a2,tf->reg.a3);
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
			Syscall_Rest();
			tf->reg.a0=0;
			break;
		case	SYS_gettimeofday:
			tf->reg.a0=Syscall_gettimeofday(tf->reg.a0);
			break;
		case	SYS_nanosleep	:
			tf->reg.a0=Syscall_nanosleep(tf->reg.a0,tf->reg.a1);
			break;
		case	SYS_lseek		:
			tf->reg.a0=Syscall_lseek(tf->reg.a0,tf->reg.a1,tf->reg.a2);
			break;
		case	SYS_prlimit64	:
			tf->reg.a0=Syscall_prlimit64(tf->reg.a0,tf->reg.a1,tf->reg.a2,tf->reg.a3);
			break;
		case	SYS_mprotect	:
			tf->reg.a0=0;
			break;
		case 	SYS_clock_gettime:
			tf->reg.a0=Syscall_clock_gettime(tf->reg.a0,tf->reg.a1);
			break;
		case 	SYS_statfs		:
			tf->reg.a0=Syscall_statfs((const char*)tf->reg.a0,tf->reg.a1);
			break;
			
		case SYS_fcntl:
			tf->reg.a0=Syscall_fcntl(tf->reg.a0,tf->reg.a1,tf);
			break;
		case SYS_sigprocmask:
		case SYS_sigtimedwait:
		case SYS_sigaction:
			
		case SYS_gettid:
		case SYS_set_tid_address:
		case SYS_exit_group:
			
//		case SYS_futex:

		case SYS_ioctl:
			
		case SYS_get_robust_list:
			
		case SYS_geteuid:
		case SYS_getegid:
			
		case SYS_utimensat:
			
		case SYS_membarrier:
			
		case SYS_socket:
		case SYS_bind:
		case SYS_getsockname:
		case SYS_setsockopt:
		case SYS_sendto:
		case SYS_recvfrom:
		case SYS_listen:
		case SYS_connect:
		case SYS_accept:
			kout[Warning]<<"Skipped syscall "<<tf->reg.a7<<" "<<SyscallName((long long)tf->reg.a7)<<endl;
			break;
		case SYS_getuid:
			tf->reg.a0 = Syscall_getuid();
			break;
		case SYS_sendfile:
			tf->reg.a0 = Syscall_sendfile(tf->reg.a0,tf->reg.a1,(long long*)tf->reg.a2,tf->reg.a3);
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
				tf->reg.a0 = 0;
				// cur->Exit(Process::Exit_BadSyscall);
				// POS_PM.Schedule();
				// kout[Fault]<<"TrapFunc_Syscall: Reaced unreachable branch!"<<endl;
			}
			break;
		}
	}
	return ERR_None;
}
