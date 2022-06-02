#ifndef POS_ELF_HPP
#define POS_ELF_HPP

#include "Process.hpp"
#include "../File/FileSystem.hpp"

template <class AddrType> struct ELF_HeaderXX
{
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

using ELF_Header32=ELF_HeaderXX <Uint32>;
using ELF_Header64=ELF_HeaderXX <Uint64>;

struct ThreadData_CreateProcessFromELF
{
	FileHandle *file;
	Process *proc;
	VirtualMemorySpace *vms;
	Semaphore sem;
	ELF_Header64 header;
	
	ThreadData_CreateProcessFromELF():sem(0) {}
};

inline int Thread_CreateProcessFromELF(void *userdata)
{
	using namespace POS;
	ThreadData_CreateProcessFromELF *d=(ThreadData_CreateProcessFromELF*)userdata;
	FileHandle *file=d->file;
	Process *proc=d->proc;
	VirtualMemorySpace *vms=d->vms;
	
	vms->EnableAccessUser();
	for (int i=0;i<d->header.phnum;++i)
	{
		ELF_ProgramHeader64 ph{0};
		file->Seek(d->header.phoff+i*d->header.phentsize,FileHandle::Seek_Beg);
		Sint64 err=file->Read(&ph,sizeof(ph));
		if (err!=sizeof(ph))
			kout[Fault]<<"Failed to read elf program header "<<file<<" ,error code "<<-err<<endl;
		bool continue_flag=0;
		switch (ph.type)
		{
			case ELF_ProgramHeader64::PT_LOAD:
				break;
			default:
				if (POS::InRange(ph.type,ELF_ProgramHeader64::PT_LOPROC,ELF_ProgramHeader64::PT_HIPROC))
				{
					kout[Warning]<<"Currently unsolvable elf segment "<<ph.type<<", do nothing..."<<endl;
					continue_flag=1;
				}
				else kout[Fault]<<"Unsolvable elf segment "<<ph.type<<endline<<ph<<endl;
		}
		if (continue_flag)
			continue;
		
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
		MemsetT<char>((char*)ph.vaddr,0,ph.memsize);
		
		file->Seek(ph.offset,FileHandle::Seek_Beg);
		err=file->Read((void*)ph.vaddr,ph.filesize);
		if (err!=ph.filesize)
			kout[Fault]<<"Failed to read elf segment "<<file<<" ,error code "<<-err<<endl;
	}
	{
		VirtualMemoryRegion *vmr_stack=KmallocT<VirtualMemoryRegion>();
		vmr_stack->Init(InnerUserProcessStackAddr,InnerUserProcessStackAddr+InnerUserProcessStackSize,VirtualMemoryRegion::VM_USERSTACK);
		vms->InsertVMR(vmr_stack);
		MemsetT<char>((char*)InnerUserProcessStackAddr,0,InnerUserProcessStackSize);//!!??
	}
	vms->DisableAccessUser();
	d->sem.Signal();
	return 0;
}

inline PID CreateProcessFromELF(FileHandle *file,Uint64 flags,const char *workDir)
{
	using namespace POS;
	kout[Info]<<"CreateProcessFromELF "<<file<<endl;
	ThreadData_CreateProcessFromELF *d=new ThreadData_CreateProcessFromELF();
	d->file=file;
	Sint64 err=file->Read(&d->header,sizeof(d->header));
	if (err!=sizeof(d->header)||!d->header.IsELF())
	{
		kout[Error]<<"CreateProcessFromELF "<<file<<" is not elf file!"<<endl;
		delete d;
		return Process::InvalidPID;
	}
	//<<Chech other info??
	
	ISASBC
	VirtualMemorySpace *vms=KmallocT<VirtualMemorySpace>();
	vms->Init();
	vms->Create(VirtualMemorySpace::VMS_Default);
	
	Process *proc=POS_PM.AllocProcess();
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
	proc->Start(Thread_CreateProcessFromELF,d,d->header.entry);
	kout[Info]<<"CreateProcessFromELF "<<file<<" OK, PID "<<proc->GetPID()<<endl;
	d->sem.Wait();
	delete d;
	return flags&Process::F_AutoDestroy?Process::InvalidPID:proc->GetPID();
}

#endif
