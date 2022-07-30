#ifndef POS_ELF_HPP
#define POS_ELF_HPP

#include "Process.hpp"
#include "../File/FileSystem.hpp"

template <class AddrType> struct ELF_HeaderXX
{
	enum
	{
		ET_NONE=0,
		ET_REL=1,
		ET_EXEC=2,
		ET_DYN=3,//Dynamic library
		ET_CORE=4,//Unsupportted yet.
		ET_LOPROC=0xFF00,
		ET_HIPROC=0xFFFF
	};
	
	union
	{
		Uint8 ident[16];//Magic number and other info
		struct
		{
			Uint8 magic[4];
			Uint8 elfClass,
				  dataEndian,
				  elfVersion;
			Uint8 padding[9];
		};
	};
	Uint16 type;//Object file type
	Uint16 machine;//Architecture
	Uint32 version;//Object file version
	AddrType entry;//Entry point virtual address
	AddrType phoff;//Program header table file offset
	AddrType shoff;//Section header table file offset
	Uint32 flags;//Processor-specific flags
	Uint16 ehsize;//ELF header size in bytes
	Uint16 phentsize;//Program header table entry size
	Uint16 phnum;//Program header table entry count
	Uint16 shentsize;//Section header table entry size
	Uint16 shnum;//Section header table entry count
	Uint16 shstrndx;//Section header string table index
	
	inline bool IsELF() const
	{return magic[0]==0x7F&&magic[1]=='E'&&magic[2]=='L'&&magic[3]=='F';}
}__attribute__((packed));

struct ELF_ProgramHeader32
{
	enum
	{
		PT_NULL=0,//Empty segment
		PT_LOAD=1,//Loadable segment
		PT_DYNAMIC=2,//Segment include dynamic linker info
		PT_INTERP=3,//Segment specified dynamic linker
		PT_NOTE=4,//Segment include compiler infomation
		PT_SHLIB=5,//Shared library segment
		PT_LOPROC=0x70000000,
		PT_HIPROC=0x7FFFFFFF
	};
	
	enum
	{
		PF_X=1,
		PF_W=2,
		PF_R=4,
		PF_MASKPROC=0xF0000000
	};
	
	Uint32 type;//Segment type
	Uint32 offset;//Segment file offset
	Uint32 vaddr;//Segment virtual address
	Uint32 paddr;//Segment physical address(used in system without virtual memory)
	Uint32 filesize;//Segment size in file
	Uint32 memsize;//Segment size in memory
	Uint32 flags;//Segment flags
	Uint32 align;//Segment alignment
}__attribute__((packed));

struct ELF_ProgramHeader64
{
	enum
	{
		PT_NULL=0,//Empty segment
		PT_LOAD=1,//Loadable segment
		PT_DYNAMIC=2,//Segment include dynamic linker info
		PT_INTERP=3,//Segment specified dynamic linker
		PT_NOTE=4,//Segment include compiler infomation
		PT_SHLIB=5,//Shared library segment
		PT_PHDR=6,
		PT_LOPROC=0x70000000,
		PT_HIPROC=0x7fffffff
	};
	
	enum
	{
		PF_X=1,
		PF_W=2,
		PF_R=4,
		PF_MASKPROC=0xf0000000
	};
	
	Uint32 type;//Segment type
	Uint32 flags;
	Uint64 offset;//Segment file offset
	Uint64 vaddr;//Segment virtual address
	Uint64 paddr;//Segment physical address(used in system without virtual memory)
	Uint64 filesize;//Segment size in file
	Uint64 memsize;//Segment size in memory
	Uint64 align;//Segment alignment
}__attribute__((packed));

enum class ELF_AT
{
	NULL	=0,
	IGNORE	=1,
	EXECFD	=2,
	PHDR	=3,
	PHENT	=4,
	PHNUM	=5,
	PAGESZ	=6,
	BASE	=7,
	FLAGS	=8,
	ENTRY	=9,
	NOTELF	=10,
	UID		=11,
	EUID	=12,
	GID		=13,
	EGID	=14,
	HWCAP	=16,
	CLKTCK	=17,
	SECURE	=23,
	EXECFN	=31
};

using ELF_Header32=ELF_HeaderXX <Uint32>;
using ELF_Header64=ELF_HeaderXX <Uint64>;

struct ThreadData_CreateProcessFromELF
{
	FileHandle *file;
	Process *proc;
	VirtualMemorySpace *vms;
	int argc;
	char **argv;
	Semaphore sem;
	ELF_Header64 header;
	
	ThreadData_CreateProcessFromELF():sem(0) {}
};

inline int Thread_CreateProcessFromELF(void *userdata)
{
	CALLINGSTACK
	using namespace POS;
	ThreadData_CreateProcessFromELF *d=(ThreadData_CreateProcessFromELF*)userdata;
	FileHandle *file=d->file;
	Process *proc=d->proc;
	VirtualMemorySpace *vms=d->vms;
	
	Uint64 ProgramInterpreterBase=0,
		   ProgramHeaderAddress=0;
	
	vms->EnableAccessUser();
	PtrInt BreakPoint=0;
	for (int i=0;i<d->header.phnum;++i)
	{
		ELF_ProgramHeader64 ph{0};
		file->Seek(d->header.phoff+i*d->header.phentsize,FileHandle::Seek_Beg);
		Sint64 err=file->Read(&ph,sizeof(ph));
		if (err!=sizeof(ph))
			kout[Fault]<<"Failed to read elf program header "<<file<<" ,error code "<<-err<<endl;
		switch (ph.type)
		{
			case ELF_ProgramHeader64::PT_LOAD:
			{
				Uint64 flags=0;
				if (ph.flags&ELF_ProgramHeader64::PF_R)
					flags|=VirtualMemoryRegion::VM_Read;
		//		if (ph.flags&ELF_ProgramHeader64::PF_W)
					flags|=VirtualMemoryRegion::VM_Write;
				if (ph.flags&ELF_ProgramHeader64::PF_X)
					flags|=VirtualMemoryRegion::VM_Exec;
				
				kout[Test]<<"Add VMR "<<(void*)ph.vaddr<<" "<<(void*)(ph.vaddr+ph.memsize)<<" "<<(void*)flags<<endl;
				auto vmr=KmallocT<VirtualMemoryRegion>();
				vmr->Init(ph.vaddr,ph.vaddr+ph.memsize,flags);
				vms->InsertVMR(vmr);
				BreakPoint=maxN(BreakPoint,vmr->GetEnd());
				
				file->Seek(ph.offset,FileHandle::Seek_Beg);
				err=file->Read((void*)ph.vaddr,ph.filesize);
				if (err!=ph.filesize)
					kout[Fault]<<"Failed to read elf segment "<<file<<" ,error code "<<-err<<endl;
				break;
			}
			case ELF_ProgramHeader64::PT_INTERP://Need improve...
			{
				file->Seek(ph.offset,FileHandle::Seek_Beg);
				char *interpPath=new char[ph.filesize];
				err=file->Read((void*)interpPath,ph.filesize);
				if (err==ph.filesize)
				{
					FileHandle *file=nullptr;
					char *path=VFSM.NormalizePath(interpPath);
					FileNode *node=VFSM.Open(path);
					delete[] path;
					if (node!=nullptr)
						file=new FileHandle(node);
					if (file!=nullptr)
					{
						ELF_Header64 header{0};
						err=file->Read(&header,sizeof(header));
						if (err!=sizeof(header))
							kout[Fault]<<"Failed to read interpreter elf header! Error code "<<err<<endl;
						Uint64 needSize=0;
						for (int i=0;i<header.phnum;++i)
						{
							ELF_ProgramHeader64 ph{0};
							file->Seek(header.phoff+i*header.phentsize,FileHandle::Seek_Beg);
							Sint64 err=file->Read(&ph,sizeof(ph));
							if (err!=sizeof(ph))
								kout[Fault]<<"Failed to read interpreter elf program header "<<file<<" ,error code "<<-err<<endl;
							if (ph.type==ELF_ProgramHeader64::PT_LOAD)
								needSize+=ph.memsize;
						}
						PtrInt s=ProgramInterpreterBase=vms->GetUsableVMR(0x60000000,0x70000000,needSize);
						{
							TrapFrame *tf=(TrapFrame*)(proc->Stack+proc->StackSize)-1;
							tf->epc=s+header.entry;
							kout[Test]<<"Set entry point as "<<(void*)tf->epc<<endl;
						}
						for (int i=0;i<header.phnum;++i)
						{
							ELF_ProgramHeader64 ph{0};
							file->Seek(header.phoff+i*header.phentsize,FileHandle::Seek_Beg);
							Sint64 err=file->Read(&ph,sizeof(ph));
							if (err!=sizeof(ph))
								kout[Fault]<<"Failed to read interpreter elf program header "<<file<<" ,error code "<<-err<<endl;
							if (ph.type==ELF_ProgramHeader64::PT_LOAD)
							{
								Uint64 flags=0;
								if (ph.flags&ELF_ProgramHeader64::PF_R)
									flags|=VirtualMemoryRegion::VM_Read;
						//		if (ph.flags&ELF_ProgramHeader64::PF_W)
									flags|=VirtualMemoryRegion::VM_Write;
								if (ph.flags&ELF_ProgramHeader64::PF_X)
									flags|=VirtualMemoryRegion::VM_Exec;
								
								kout[Test]<<"Add VMR of INTERP "<<(void*)(s+ph.vaddr)<<" "<<(void*)(s+ph.vaddr+ph.memsize)<<" "<<(void*)flags<<endl;
								auto vmr=KmallocT<VirtualMemoryRegion>();
								vmr->Init(s+ph.vaddr,s+ph.vaddr+ph.memsize,flags);
								vms->InsertVMR(vmr);
								
								file->Seek(ph.offset,FileHandle::Seek_Beg);
								err=file->Read((void*)(s+ph.vaddr),ph.filesize);
								if (err!=ph.filesize)
									kout[Fault]<<"Failed to read elf segment "<<file<<" ,error code "<<-err<<endl;
							}
						}
					}
					else kout[Fault]<<"Failed to read elf INTERP path "<<interpPath<<endl;
					delete file;
					VFSM.Close(node);
				}
				else kout[Fault]<<"Failed to read elf INTERP path! Error code "<<-err<<endl;
				delete[] interpPath;
				break;
			}
			case ELF_ProgramHeader64::PT_PHDR:
				ProgramHeaderAddress=ph.vaddr;
				break;
			case 1685382481://Aka GNU_STACK
			case 1685382482://Aka GNU_RELRO
			case 7://Aka TLS
			case ELF_ProgramHeader64::PT_DYNAMIC:
				break;
			default:
				if (POS::InRange(ph.type,ELF_ProgramHeader64::PT_LOPROC,ELF_ProgramHeader64::PT_HIPROC))
					kout[Warning]<<"Currently unsolvable elf segment "<<ph.type<<", do nothing..."<<endl;
				else kout[Fault]<<"Unsolvable elf segment "<<ph.type<<endline<<ph<<endl;
				break;
		}
	}
	{
		VirtualMemoryRegion *vmr_stack=KmallocT<VirtualMemoryRegion>();
		vmr_stack->Init(InnerUserProcessStackAddr,InnerUserProcessStackAddr+InnerUserProcessStackSize,VirtualMemoryRegion::VM_USERSTACK);
		vms->InsertVMR(vmr_stack);
	}
	{
		HeapMemoryRegion *hmr=KmallocT<HeapMemoryRegion>();
		hmr->Init(BreakPoint);
		vms->InsertVMR(hmr);
		proc->SetHeap(hmr);
	}
	{//Init info //Testing...
		TrapFrame *tf=(TrapFrame*)(proc->Stack+proc->StackSize)-1;
		Uint8 *sp=(decltype(sp))tf->reg.sp;
		auto PushInfo32=[&sp](Uint32 info)
		{
			*(Uint32*)sp=info;
			sp+=sizeof(long);//!! Need improve...
		};
		auto PushInfo64=[&sp](Uint64 info)
		{
			*(Uint64*)sp=info;
			sp+=8;
		};
		auto AddAUX=[&PushInfo64](ELF_AT at,Uint64 value)
		{
			PushInfo64((Uint64)at);
			PushInfo64(value);
		};
		PtrInt p=vms->GetUsableVMR(0x60000000,0x70000000,PageSize);
		VirtualMemoryRegion *vmr_str=KmallocT<VirtualMemoryRegion>();
		vmr_str->Init(p,p+PageSize,VirtualMemoryRegion::VM_RW);
		vms->InsertVMR(vmr_str);
		char *s=(char*)p;
		auto PushString=[&s](const char *str)->const char*
		{
			const char *s_bak=s;
			s=strCopyRe(s,str);
			*s++=0;
			return s_bak;
		};
	
		PushInfo32(d->argc);
		if (d->argc)
			for (int i=0;i<d->argc;++i)
				PushInfo64((Uint64)PushString(d->argv[i]));
		PushInfo64(0);//End of argv
		PushInfo64((Uint64)PushString("LD_LIBRARY_PATH=/VFS/FAT32"));
		PushInfo64(0);//End of envs
		if (ProgramHeaderAddress!=0)
		{
			AddAUX(ELF_AT::PHDR,ProgramHeaderAddress);
			AddAUX(ELF_AT::PHENT,d->header.phentsize);
			AddAUX(ELF_AT::PHNUM,d->header.phnum);
			
			AddAUX(ELF_AT::UID,10);
			AddAUX(ELF_AT::EUID,10);
			AddAUX(ELF_AT::GID,10);
			AddAUX(ELF_AT::EGID,10);
		}
		AddAUX(ELF_AT::PAGESZ,PageSize);
		if (ProgramInterpreterBase!=0)
			AddAUX(ELF_AT::BASE,ProgramInterpreterBase);
		AddAUX(ELF_AT::ENTRY,d->header.entry);
		PushInfo64(0);//End of auxv
	}
	vms->DisableAccessUser();
	d->sem.Signal();
	kout[Info]<<"CreateProcessFromELF "<<file<<" OK, PID "<<proc->GetPID()<<endl;
	return 0;
}

inline PID CreateProcessFromELF(FileHandle *file,Uint64 flags,const char *workDir,int argc=0,char **argv=nullptr)
{
	using namespace POS;
	kout[Info]<<"CreateProcessFromELF "<<file<<endl;
	ThreadData_CreateProcessFromELF *d=new ThreadData_CreateProcessFromELF();
	d->file=file;
	Sint64 err=file->Read(&d->header,sizeof(d->header));
	if (err!=sizeof(d->header)||!d->header.IsELF())
	{
		kout[Error]<<"CreateProcessFromELF "<<file<<", it is not elf file!"<<endl;
		delete d;
		return Process::InvalidPID;
	}
	if (d->header.type!=ELF_Header64::ET_EXEC)
	{
		kout[Error]<<"CreateProcessFromELF "<<file<<", ELF type "<<d->header.type<<" beyond ET_EXEC is not supported yet!"<<endl;
		delete d;
		return Process::InvalidPID;
	}
	//<<Chech other info??
	
	VirtualMemorySpace *vms=KmallocT<VirtualMemorySpace>();
	vms->Init();
	vms->Create(VirtualMemorySpace::VMS_Default);
	
	Process *proc=POS_PM.AllocProcess();
	PID re=proc->GetPID();
	proc->Init(flags);
	proc->SetStack(nullptr,KernelStackSize);
	proc->SetVMS(vms);
	if (!(flags&Process::F_AutoDestroy))
		proc->SetFa(POS_PM.Current());
	
	if (VFSM.IsAbsolutePath(workDir))
		proc->SetCWD(workDir);
	else
	{
		char *cwd=VFSM.NormalizePath(workDir,POS_PM.Current()->GetCWD());
		proc->SetCWD(cwd);
		Kfree(cwd);
	}
	
	d->vms=vms;
	d->proc=proc;
	d->argc=argc;
	d->argv=argv;
	proc->Start(Thread_CreateProcessFromELF,d,d->header.entry);
	
	d->sem.Wait();
	delete d;
	return flags&Process::F_AutoDestroy?Process::UnknownPID:re;
}

#endif
