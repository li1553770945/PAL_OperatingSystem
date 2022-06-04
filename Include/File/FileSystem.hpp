#ifndef POS_FILESYSTEM_HPP
#define POS_FILESYSTEM_HPP

#include "../Error.hpp"
#include "../Library/TemplateTools.hpp"
#include "../Process/Synchronize.hpp"
#include "../Library/String/SysStringTools.hpp"
#include "../Library/Kout.hpp"
#include "../Library/DataStructure/LinkTable.hpp"
#include "FilePathTools.hpp"
#include "../Library/DataStructure/PAL_Tuple.hpp"

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
	friend class VirtualFileSystemManager;
	public:
		enum:Uint64
		{
			A_Dir		=1ull<<0,
			A_Root		=1ull<<1,
			A_VFS		=1ull<<2,//root of VFS
			A_Device	=1ull<<3,
//			A_Virtual	=1ull<<4,
//			A_Deleted	=1ull<<,
//			A_Link		=1ull<<,
		};
		
		enum:Uint64
		{
			F_OutsideName=1ull<<0,//Means that the name if not dumplicated and won't be freed.(Also cannot modify!)
			F_BelongVFS  =1ull<<1,//if in the subtree of VFS
			F_Managed    =1ull<<2,
			F_Base       =1ull<<3,
			F_AutoClose  =1ull<<4
		};
		
	protected:
		VirtualFileSystem *Vfs=nullptr;//Belonging VFS
		char *Name=nullptr;
		Uint64 Attributes=0;
		Uint64 Flags=0;
		FileNode *fa=nullptr,
				 *pre=nullptr,
				 *nxt=nullptr,
				 *child=nullptr;
		Uint64 FileSize=0;
		Sint32 RefCount=0;
		
		virtual inline void SetFileName(char *name,bool outside)
		{
			//<<Validate name...??
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
		
		void SetFa(FileNode *_fa)
		{
			ASSERTEX(_fa==nullptr||(_fa->Attributes&A_Dir),"FileNode::SetFa "<<_fa<<" of "<<Name<<" is not valid!");
			if (fa!=nullptr)
			{
				if (fa->child==this)
					fa->child=nxt;
				else if (pre!=nullptr)
					pre->nxt=nxt;
				if (nxt!=nullptr)
					nxt->pre=pre;
				pre=nxt=fa=nullptr;
			}
			if (_fa!=nullptr)
			{
				fa=_fa;
				nxt=fa->child;
				fa->child=this;
				if (nxt!=nullptr)
					nxt->pre=this;
			}
		}
		
		template <int type> Uint64 GetPath_Measure()
		{
			if (Attributes&A_Root)
				return 0;
			else if (type==1&&(Attributes&A_VFS))
				return 0;
			else if (fa==nullptr)
				return 0;
			else return fa->GetPath_Measure<type>()+1+POS::strLen(Name);
		}
		
		template <int type> char* GetPath_Copy(char *dst)
		{
			if (Attributes&A_Root)
				return dst;
			else if (type==1&&(Attributes&A_VFS))
				return dst;
			else if (fa==nullptr)
				return dst;
			else
			{
				char *s=fa->GetPath_Copy<type>(dst);
				*s='/';
				return POS::strCopyRe(s+1,Name);
			}
		}
		
	public:
		virtual Sint64 Read(void *dst,Uint64 pos,Uint64 size)
		{return ERR_UnsuppoertedVirtualFunction;}
		
		virtual Sint64 Write(void *src,Uint64 pos,Uint64 size)
		{return ERR_UnsuppoertedVirtualFunction;}
		
		template <int type> char* GetPath()//type 0:Fullpath 1:Path in VFS;returned string should be freed by caller
		{
			Uint64 len=GetPath_Measure<type>();
			if (len==0)
				return POS::strDump("/");
			char *re=(char*)Kmalloc(len+1);
			*GetPath_Copy<type>(re)=0;
			return re;
		}
		
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
			if (RefCount<=0&&(Flags&F_AutoClose))
				delete this;
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
			while (child)
				delete child;
			SetFa(nullptr);
			SetFileName(nullptr,0);
		}
		
		FileNode(VirtualFileSystem *_vfs,Uint64 attri,Uint64 flags):Vfs(_vfs),Attributes(attri),Flags(flags)
		{
			if (Vfs!=nullptr)
				Flags|=F_BelongVFS;
		}
};

class FileHandle:public POS::LinkTableT<FileHandle>
{
	friend class Process;
	public:
		enum
		{
			F_Read	=1ull<<0,
			F_Write	=1ull<<1,
			F_Seek	=1ull<<2,
			F_Size	=1ull<<3, 
			
			F_ALL   =F_Read|F_Write|F_Seek|F_Size
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
		
	public:
		inline Sint64 Read(void *dst,Uint64 size)//Need improve...
		{
			if (!(Flags&F_Read))
				return -ERR_InvalidFileHandlePermission;
			auto err=file->Read(dst,Pos,size);
			if (err>=0)
				Pos+=err;
			return err;
		}
		
		inline Sint64 Write(void *src,Uint64 size)
		{
			if (!(Flags&F_Write))
				return -ERR_InvalidFileHandlePermission;
			auto err=file->Write(src,Pos,size);
			if (err>=0)
				Pos+=err;
			return err;
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
		
		inline FileNode* Node()
		{return file;}
		
		inline int GetFD()
		{return FD;}
		
		inline Uint64 GetFlags()
		{return Flags;}
		
		ErrorType Close()
		{
			ErrorType err=ERR_None;
			if (file)
			{
				err=file->Unref(this);
				file=nullptr;
			}
			if (proc)
			{
				if (FD<=7)
					proc->FileTable[FD]=nullptr;
				Remove();
				FD=-1;
				proc=nullptr;
			}
			return err;
		}
		
		ErrorType BindToProcess(Process *_proc,int fd=-1)//if fd==-1, auto allocate fd; fd should not be used if specified!
		{
			if (proc!=nullptr)
				return ERR_TargetExist;
			proc=_proc;
			if (fd==0||fd==1)//For special use in Process::CopyFileTable; Won't check existance! (??)
			{
				FD=fd;
				proc->FileTable[fd]=this;
				if (fd==0&&proc->FileTable[1])
					proc->FileTable[1]->PreInsert(this);
				if (fd==1&&proc->FileTable[0])
					proc->FileTable[0]->NxtInsert(this);
				return ERR_None;
			}
			FileHandle *p=proc->FileTable[1];
			while (p)
				if (fd==-1)
					if (p->nxt==nullptr||p->nxt->FD>p->FD+1)
					{
						fd=p->FD+1;
						break;
					}
					else p=p->nxt;
				else
					if (p->FD<fd)
						if (p->nxt==nullptr||fd<p->nxt->FD)
							break;
						else p=p->nxt;
					else if (p->FD==fd)
						return ERR_TargetExist;
			FD=fd;
			p->NxtInsert(this);
			if (fd<=7)
				proc->FileTable[fd]=this;
			return ERR_None;
		}
		
		FileHandle* Dump()//??
		{
			FileHandle *re=new FileHandle(file,Flags);
			re->Pos=Pos;
			return re;
		}
		
		~FileHandle()
		{
			Close();
		}
		
		FileHandle(FileNode *filenode,Uint64 flags=F_ALL):Flags(flags)
		{
			file=filenode;
			file->Ref(this);
			LinkTableT<FileHandle>::Init();
		}
};

class VirtualFileSystemManager
{
	protected:
		FileNode *root;
		
		void AddNewNode(FileNode *p,FileNode *fa);
		FileNode* AddFileInVFS(FileNode *p,char *name);
		FileNode* FindChildName(FileNode *p,const char *s,const char *e);
		FileNode* FindChildName(FileNode *p,const char *s);
		FileNode* FindRecursive(FileNode *p,const char *path);
		PAL_DS::Doublet <VirtualFileSystem*,const char*> FindPathOfVFS(FileNode *p,const char *path);
		
	public:
		static inline bool IsAbsolutePath(const char *path)
		{
			return path!=nullptr&&*path=='/';//??
		}
		
		static char* NormalizePath(const char *path,const char *base=nullptr);//if base is nullptr, or path is absolute, base will be ignored.
		FileNode* FindFile(const char *path,const char *name);
		int GetAllFileIn(const char *path,char *result[],int bufferSize,int skipCnt=0);//if unused ,user should free the char*
		int GetAllFileIn(Process *proc,const char *path,char *result[],int bufferSize,int skipCnt=0);
		ErrorType CreateDirectory(const char *path);
		ErrorType CreateDirectory(Process *proc,const char *path);
		ErrorType CreateFile(const char *path);
		ErrorType CreateFile(Process *proc,const char *path);
		ErrorType Move(const char *src,const char *dst);
		ErrorType Copy(const char *src,const char *dst);
		ErrorType Delete(const char *path);
		ErrorType LoadVFS(VirtualFileSystem *vfs,const char *path="/VFS");
		
		FileNode* Open(const char *path);//path here should be normalized.
		FileNode* Open(Process *proc,const char *path);
		ErrorType Close(FileNode *p);
		
		ErrorType Init();
		ErrorType Destroy();
};
extern VirtualFileSystemManager VFSM;

class VirtualFileSystem
{
//	protected:
	public:
		virtual FileNode* FindFile(FileNode *p,const char *name)//Temp...
		{
			char *path=p->GetPath<1>();
			FileNode *re=FindFile(path,name);
			Kfree(path);
			return re;
		}
		
		virtual int GetAllFileIn(FileNode *p,char *result[],int bufferSize,int skipCnt=0)
		{
//			using namespace POS;
//	kout[Debug]<<"G9"<<endl;
			char *path=p->GetPath<1>();
//	kout[Debug]<<"G10"<<endl;
			int re=GetAllFileIn(path,result,bufferSize,skipCnt);
//	kout[Debug]<<"G11"<<endl;
			Kfree(path);
			return re;
		}
		
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
