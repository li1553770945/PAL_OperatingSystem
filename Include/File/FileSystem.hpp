#ifndef POS_FILESYSTEM_HPP
#define POS_FILESYSTEM_HPP

#include "../Error.hpp"
#include "../Library/TemplateTools.hpp"
#include "../Process/Synchronize.hpp"
#include "../Library/String/SysStringTools.hpp"

class FileNode;
class VirtualFileSystem;

inline const char * InvalidFileNameCharacter()
{return "/\\:*?\"<>|";}

inline bool IsValidFileNameCharacter(char ch)
{return POS::NotInSet(ch,'/','\\',':','*','?','\"','<','>','|');}

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
		FileNode *file;
		Uint64 Flags;
		
	public:
		Sint64 Read(void *dst,Uint64 size);
		Sint64 Write(void *src,Uint64 size);
		ErrorType Seek(Uint64 pos);
		Uint64 Size();
		ErrorType Close();
};

class FileNode
{
	
	public:
		enum:Uint64
		{
			Attri_Dir		=1ull<<0,
//			Attri_Deleted	=1ull<<1,
//			Attri_Link		=1ull<<2,
//			Attri_VFS		=1ull<<3,
		};
		
	protected:
		VirtualFileSystem *Vfs;//Belonging VFS
		char *Name;
		Uint64 Attributes;
//		FileNode *fa;
		Uint64 FileSize;
		Uint64 RefCount;
		
	public:
		virtual ErrorType Read(void *dst,Uint64 pos,Uint64 size);
		virtual ErrorType Write(void *src,Uint64 pos,Uint64 size);
		
		inline Uint64 Size()
		{return FileSize;}
		
		inline ErrorType Ref(FileHandle *f)
		{
			//...
			++RefCount;
		}
		
		inline ErrorType Unref(FileHandle *f)
		{
			//...
			--RefCount;
		}
		
		virtual ~FileNode()
		{
			if (Name!=nullptr)
				Kfree(Name);
		}
		
		FileNode(VirtualFileSystem *_vfs=nullptr,const char *_name=nullptr,Uint64 attri=0):Vfs(_vfs),Attributes(attri)
		{
			if (_name==nullptr)
				Name=nullptr;
			else Name=POS::strDump(_name);
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
	public://Path parameter in VFS is relative path to the VFS root.
		virtual const char *FileSystemName()=0;
		virtual FileNode* FindFile(const char *path,const char *name)=0;
		virtual int GetAllFileIn(const char *path,char *result[],int bufferSize)=0;//if unused ,user should free the char*
		virtual ErrorType CreateDirectory(const char *path)=0;
		virtual ErrorType CreateFile(const char *path)=0;
		virtual ErrorType Move(const char *src,const char *dst)=0;
		virtual ErrorType Copy(const char *src,const char *dst)=0;
		virtual ErrorType Delete(const char *path)=0;
		
		virtual FileNode* Open(const char *path)=0;
		virtual ErrorType Close(FileNode *p)=0;
		
		virtual ~VirtualFileSystem()=0;
};

#endif
