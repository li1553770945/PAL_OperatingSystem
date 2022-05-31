#ifndef POS_FILESYSTEM_HPP
#define POS_FILESYSTEM_HPP

#include "../Error.hpp"
#include "../Library/TemplateTools.hpp"
#include "../Process/Synchronize.hpp"
#include "../Library/String/SysStringTools.hpp"
#include "../Library/Kout.hpp"
#include "../Library/DataStructure/LinkTable.hpp"
#include "FilePathTools.hpp"

class FileHandle;
class FileNode;
class VirtualFileSystem;
class VirtualFileSystemManager;
class Process;

inline const char * InvalidFileNameCharacter()
{return "/\\:*?\"<>|";}

inline bool IsValidFileNameCharacter(char ch)
{return POS::NotInSet(ch,'/','\\',':','*','?','\"','<','>','|');}

class FileNode
{
	
	public:
		enum:Uint64
		{
			A_Dir		=1ull<<0,
			A_VFS		=1ull<<1,//root of VFS
//			A_Deleted	=1ull<<,
//			A_Link		=1ull<<,
		};
		
		enum:Uint64
		{
			F_OutsideName=1ull<<0//Means that the name if not dumplicated and won't be freed.
		};
		
	protected:
		VirtualFileSystem *Vfs=nullptr;//Belonging VFS
		char *Name=nullptr;
		Uint64 Attributes=0;
		Uint64 Flags=0;
//		FileNode *fa;
		Uint64 FileSize=0;
		Uint64 RefCount=0;
		
		virtual inline void SetFileName(char *name,bool outside)
		{
			if (Name!=nullptr&&!(Flags&F_OutsideName))
				Kfree(Name);
			if (outside)
			{
				Flags|=F_OutsideName;
				Name=name;
			}
			else
			{
				Flags&=~F_OutsideName;
				Name=POS::strDump(name);
			}
		}
		
	public:
		virtual ErrorType Read(void *dst,Uint64 pos,Uint64 size)=0;
		virtual ErrorType Write(void *src,Uint64 pos,Uint64 size)=0;
		
		virtual inline Uint64 Size()
		{return FileSize;}
		
		virtual inline ErrorType Ref(FileHandle *f)
		{
			//...
			++RefCount;
			return ERR_None;
		}
		
		virtual inline ErrorType Unref(FileHandle *f)
		{
			//...
			--RefCount;
			return ERR_None;
		}
		
		inline const char* GetName() const
		{return Name;}
		
		inline Uint64 GetAttributes() const
		{return Attributes;}
		
		inline bool IsDir() const
		{return Attributes&A_Dir;}
		
		virtual ~FileNode()
		{
			SetFileName(nullptr,0);
		}
		
		FileNode(VirtualFileSystem *_vfs=nullptr):Vfs(_vfs) {}
};

class FileHandle
{
	public:
		enum
		{
			F_Read	=1ull<<0,
			F_Write	=1ull<<1,
			F_Seek	=1ull<<2,
			F_Size	=1ull<<3
		};
	
		enum
		{
			Seek_Beg=0,
			Seek_Cur,
			Seek_End,
		};
	
	protected:
		FileNode *file=nullptr;
		Uint64 Pos=0;
		Uint64 Flags=0;
		
		//Below is infomation for process
		Process *proc=nullptr;
		Uint32 FD=-1;
		POS::LinkTable <Process> Link;
		
	public:
		inline Sint64 Read(void *dst,Uint64 size)//Need improve...
		{
			if (!(Flags&F_Read))
				return -ERR_InvalidFileHandlePermission;
			auto err=file->Read(dst,Pos,size);
			if (err)
				return -err;
			else return Pos+=size,size;
		}
		
		inline Sint64 Write(void *src,Uint64 size)
		{
			if (!(Flags&F_Write))
				return -ERR_InvalidFileHandlePermission;
			auto err=file->Write(src,Pos,size);
			if (err)
				return -err;
			else return Pos+=size,size;
		}
		
		inline ErrorType Seek(Sint64 pos,Uint8 base=Seek_Beg)
		{
			if (!(Flags&F_Seek))
				return -ERR_InvalidFileHandlePermission;
			switch (base)
			{
				case Seek_Beg: Pos=pos;  		break;
				case Seek_Cur: Pos+=pos; 		break;
				case Seek_End: Pos=file->Size();break;//??
				default:	return ERR_InvalidParameter;
			}
			return ERR_None;
		}
		
		inline Uint64 Size()
		{
			if (!(Flags&F_Size))
				return -ERR_InvalidFileHandlePermission;
			return file->Size();
		}
		
		ErrorType Close()
		{
			file->Unref(this);
			file=nullptr;
			return ERR_None;
		}
		
		~FileHandle()
		{
			using namespace POS;
			kout[Warning]<<"FileHandle deconstructor is not complete yet!"<<endl;
		}
		
		FileHandle(FileNode *filenode,Uint64 flags=F_Read|F_Write|F_Seek|F_Size):Flags(flags)
		{
			file=filenode;
			file->Ref(this);
		}
};

class VirtualFileSystemManager
{
	protected:
		Mutex mu;
		
	public:
		FileNode* FindFile(const char *path,const char *name);
		int GetAllFileIn(const char *path,char *result[],int bufferSize);//if unused ,user should free the char*
		ErrorType CreateDirectory(const char *path);
		ErrorType CreateFile(const char *path);
		ErrorType Move(const char *src,const char *dst);
		ErrorType Copy(const char *src,const char *dst);
		ErrorType Delete(const char *path);
		
		FileNode* Open(const char *path);
		ErrorType Close(FileNode *p);
		
		ErrorType Init();
		ErrorType Destroy();
};
extern VirtualFileSystemManager VFSM;

class VirtualFileSystem
{
//	protected:
	public:
		virtual FileNode* FindFile(const char *path,const char *name)=0;
		virtual int GetAllFileIn(const char *path,char *result[],int bufferSize,int skipCnt=0)=0;//if unused,result should be empty when input , user should free the char*
		virtual ErrorType CreateDirectory(const char *path)=0;
		virtual ErrorType CreateFile(const char *path)=0;
		virtual ErrorType Move(const char *src,const char *dst)=0;
		virtual ErrorType Copy(const char *src,const char *dst)=0;
		virtual ErrorType Delete(const char *path)=0;
		virtual FileNode* GetNextFile(const char *base)=0;
		
		virtual FileNode* Open(const char *path)=0;
		virtual ErrorType Close(FileNode *p)=0;
		
	public://Path parameter in VFS is relative path to the VFS root.
		virtual const char *FileSystemName()=0;
		virtual ~VirtualFileSystem()
		{
			using namespace POS;
			kout[Test]<<"VirtualFileSystem Deconstruct"<<endl;
		}
		
		VirtualFileSystem() 
		{
			using namespace POS;
			kout[Test]<<"VirtualFileSystem Construct"<<endl;
		}
};

#endif
