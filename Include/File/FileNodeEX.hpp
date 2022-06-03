#ifndef POS_FILENODEEX_HPP
#define POS_FILENODEEX_HPP

#include "FileSystem.hpp"
#include "../Library/BasicFunctions.hpp"
#include "../Memory/VirtualMemory.hpp"

class UartFileNode:public FileNode
{
	public:
		virtual Sint64 Read(void *dst,Uint64 pos,Uint64 size)//pos is not used...
		{
			char *s=(char*)dst,*e=s+size;
			while (s!=e)
				*s++=POS::Getchar();//??
			return size;
		}
		
		virtual Sint64 Write(void *src,Uint64 pos,Uint64 size)//pos is not used...
		{
			char *s=(char*)src,*e=s+size;
			while (s!=e)
				POS::Putchar(*s++);
			return size;
		}
		
		UartFileNode():FileNode(nullptr,A_Device,F_Managed)
		{
			SetFileName("STDIO",1);
		}
};
extern UartFileNode *stdIO;

class PipeFileNode:public FileNode
{
	protected:
		Semaphore Lock,SemR,SemW;
		Sint32 WriterCount=0;
		char *buffer=nullptr;
		Uint64 BufferSize=0;
		Uint64 PosR=0,
			   PosW=0;
		
	public:
		virtual Sint64 Read(void *dst,Uint64 pos,Uint64 size)//pos is not used...
		{
			Uint64 size_bak=size;
			bool flag=0;
			while (size>0)
			{
				Lock.Wait();
				if (FileSize>0)
				{
					while (size>0&&FileSize>0)
					{
						*(char*)dst++=buffer[PosR++];
						--FileSize;
						--size;
						if (PosR==BufferSize)
							PosR=0;
					}
					SemW.Signal();//??
					Lock.Signal();
				}
				else
				{
					if (WriterCount==0)
						flag=1;
					Lock.Signal();
					if (flag)
					{
						SemR.Signal(Semaphore::SignalAll);
						return size_bak-size;
					}
					else SemR.Wait();
				}
			}
			return size_bak;
		}
		
		virtual Sint64 Write(void *src,Uint64 pos,Uint64 size)//pos is not used...
		{
			Uint64 size_bak=size;
			while (size>0)
			{
				Lock.Wait();
				if (FileSize<BufferSize)
				{
					while (size>0&&FileSize<BufferSize)
					{
						buffer[PosW++]=*(char*)src++;
						++FileSize;
						--size;
						if (PosW==BufferSize)
							PosW=0;
					}
					SemR.Signal();//??
					Lock.Signal();
				}
				else
				{
					Lock.Signal();
					SemW.Wait();
				}
			}
			return size_bak;
		}
		
		virtual inline ErrorType Ref(FileHandle *f)
		{
			FileNode::Ref(f);
			if (f->GetFlags()&FileHandle::F_Write)
				++WriterCount;
			return ERR_None;
		}
		
		virtual inline ErrorType Unref(FileHandle *f)
		{
			if (f->GetFlags()&FileHandle::F_Write)
				--WriterCount;
			FileNode::Unref(f);
			return ERR_None;
		}
		
		~PipeFileNode()
		{
			delete[] buffer;
		}
		
		PipeFileNode(const char *name=nullptr,Uint64 bufferSize=4096)
		:FileNode(nullptr,0,F_AutoClose),Lock(1),SemR(0),SemW(0),BufferSize(bufferSize)//??
		{
			if (name!=nullptr)
				SetFileName((char*)name,0);
			buffer=new char[BufferSize];
		}
};

class MemapFileRegion:public VirtualMemoryRegion
{
	protected:
		FileNode *File=nullptr;
		Uint64 Start=0,
			   Length=0,
			   Offset=0;
		
	public:
		ErrorType Save()//Save memory to file
		{
			VirtualMemorySpace *old=VirtualMemorySpace::Current();
			VMS->Enter();
			VMS->EnableAccessUser();
			Sint64 re=File->Write((void*)Start,Offset,Length);
			VMS->DisableAccessUser();
			old->Enter();
			return re>=0?ERR_None:-re;//??
		}
		
		ErrorType Load()//Load memory from file
		{
			VirtualMemorySpace *old=VirtualMemorySpace::Current();
			VMS->Enter();
			VMS->EnableAccessUser();
			Sint64 re=File->Read((void*)Start,Offset,Length);
			VMS->DisableAccessUser();
			old->Enter();
			return re>=0?ERR_None:-re;
		}
		
		~MemapFileRegion()//Virtual??
		{
			File->Unref(nullptr);
			if (VMS!=nullptr)
				VMS->RemoveVMR(this,0);
		}
		
		MemapFileRegion(FileNode *node,void *start,Uint64 len,Uint64 offset,Uint32 prot)
		:File(node),Start((PtrInt)start),Length(len),Offset(offset)
		{
			ASSERTEX(VirtualMemoryRegion::Init((PtrInt)start,(PtrInt)start+len,prot)==ERR_None,"MemapFileRegion "<<this<<" failed to init VMR!");
			node->Ref(nullptr);
		}
};

#endif
