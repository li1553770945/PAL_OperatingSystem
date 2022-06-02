#ifndef POS_STUPIDFILESYSTEM_HPP
#define POS_STUPIDFILESYSTEM_HPP

#include "FileSystem.hpp"
#include "../Library/String/StringTools.hpp"
#include "../Memory/PhysicalMemory.hpp"
#include "FilePathTools.hpp"
#include "../Library/Kout.hpp"

class StupidFileSystem;
class StupidFileNode:public FileNode
{
	friend class StupidFileSystem;
	public:
		enum
		{
			Attri_Dir	=1<<0,
			Attri_Root	=1<<1
		};
		
	protected:
		int ID=0,//ID represents the index in the disk,so it shouldn't be used in memory, ID will be reassigned when needed.
			FaID=0;
		int Size=0;
		int Attributes=0;
		int ChildCount=0;
		char Padding[56]{0};
		char Name[20]{0};
		char Data[4000]{0};
		
		StupidFileSystem *SFS=nullptr;
		StupidFileNode *fa=nullptr,
					   *nxt=nullptr,
					   *pre=nullptr,
					   *child=nullptr;
		
		void Insert(StupidFileNode *p)//Head insert method
		{
			p->fa=this;
			if (child==nullptr)
				child=p;
			else
			{
				child->pre=p;
				p->nxt=child;
				child=p;
			}
		}
		
		StupidFileNode* FindChild(const char *s,const char *se)
		{
			for (StupidFileNode *u=child;u;u=u->nxt)
				if (POS::strComp(s,se,u->Name)==0)
					return u;
			return nullptr;
		}
		
		StupidFileNode* FindChild(const char *s)
		{
			for (StupidFileNode *u=child;u;u=u->nxt)
				if (POS::strComp(s,u->Name)==0)
					return u;
			return nullptr;
		}
		
		ErrorType InitDataForFileNode()
		{
			FileSize=Size;
			if (Attributes&Attri_Dir)
				FileNode::Attributes|=A_Dir;
			if (Attributes&Attri_Root)
				FileNode::Attributes|=A_VFS;
			return ERR_None;
		}
		
	public:
		virtual Sint64 Read(void *dst,Uint64 pos,Uint64 size)
		{
			if (pos>=Size||pos+size>=Size)
				return -ERR_FileOperationOutofRange;
			for (Uint64 i=0;i<size;++i)
				((char*)dst)[i]=Data[pos+i];
			return size;
		}
		
		virtual Sint64 Write(void *src,Uint64 pos,Uint64 size)
		{
			if (pos>=sizeof(Data)||pos+size>=sizeof(Data))
				return -ERR_FileOperationOutofRange;
			Size=pos+size;
			for (Uint64 i=0;i<size;++i)
				Data[pos+i]=((char*)src)[i];
			return size;
		}
		
		inline ErrorType SetFileName(const char *name)
		{
			using namespace POS;
			int len=strLen(name);
			if (len>19)
				return ERR_FileNameTooLong;
			strCopy(Name,name);
			return ERR_None;
		}
		
		virtual ~StupidFileNode();//recursive...
		StupidFileNode(StupidFileSystem *_sfs);
};

class StupidFileSystem:public VirtualFileSystem
{
	/*
		Disk: |Version(4)|HeadMark(4)|Count(4)|PaddingTo4K|Entry0|Entry1|...|EntryN|TailMask(4)|FreeSpace|
		SFS can only use max free memory as disk size.
		SFS load all data into memory at first, then all operation will be done in memory, finally, it save all data back to disk.
	*/
	friend class StupidFileNode;
	protected:
		StupidFileNode *root=nullptr;
		PtrInt DiskStartInRAM=0;
		Uint64 DiskSize=0;
		const int SectorSize=512;
		int CurrentMark=0;
		int TotalFiles=0;
		
		void ReadSectors(void *dst,Uint64 LBA,Uint64 cnt=1)
		{
			ASSERTEX((LBA+cnt)*SectorSize<=DiskSize,"SFS::ReadSector invalid parameter!");
			for (Uint64 i=DiskStartInRAM+LBA*SectorSize,j=0;j<cnt*SectorSize;++i,++j)
				((char*)dst)[j]=*(char*)i;
		}
		
		void WriteSectors(void *src,Uint64 LBA,Uint64 cnt=1)
		{
			ASSERTEX((LBA+cnt)*SectorSize<=DiskSize,"SFS::ReadSector invalid parameter!");
			for (Uint64 i=DiskStartInRAM+LBA*SectorSize,j=0;j<cnt*SectorSize;++i,++j)
				*(char*)i=((char*)src)[j];
		}
		
		ErrorType LoadData()//Currently, the disk is also a region in RAMs
		{
			int version,mark,cnt;
			char sector[512];
			ReadSectors(sector,0);
			version=*(int*)sector;
			CurrentMark=*(1+(int*)sector);
			TotalFiles=*(2+(int*)sector);
			if (version!=1)
				return ERR_VersionError;
			StupidFileNode **us=new StupidFileNode*[TotalFiles];
			for (int i=0;i<TotalFiles;++i)
			{
				us[i]=new StupidFileNode(this);
				ReadSectors(&us[i]->ID,(i+1)*8,8);
				us[i]->InitDataForFileNode();
				if (i==0)
					root=us[i];
				else us[us[i]->FaID]->Insert(us[i]);
			}
			delete[] us;
			ReadSectors(sector,TotalFiles*8+8);
			mark=*(int*)sector;
			using namespace POS;
			if (mark!=CurrentMark)
				return ERR_VertifyNumberDisagree;
			return ERR_None;
		}
		
		ErrorType DFS_SaveData(StupidFileNode *u,int &ID)
		{
			u->ID=ID;
			if (u->fa!=nullptr)
				u->FaID=u->fa->ID;
			WriteSectors(&u->ID,(ID+1)*8,8);
			++ID;
			for (StupidFileNode *v=u->child;v;v=v->nxt)
				if (int err;err=DFS_SaveData(v,ID))
					return err;
			return ERR_None;
		}
		
		ErrorType SaveData()
		{
			using namespace POS;
//			kout[Test]<<"StupidFileSystem::SaveData"<<endl;
			char sector[512];
			MemsetT<char>(sector,0,sizeof(sector));
			*(int*)sector=1;
			*(1+(int*)sector)=++CurrentMark;
			*(2+(int*)sector)=TotalFiles;
			WriteSectors(sector,0);
			int NodeID=0;
			DFS_SaveData(root,NodeID);
			MemsetT<char>(sector,0,sizeof(sector));
			*(int*)sector=CurrentMark;
			WriteSectors(sector,TotalFiles*8+8);
//			kout[Test]<<"StupidFileSystem::SaveData OK"<<endl;
			return ERR_None;
		}
		
		ErrorType InitEmpty()
		{
			using namespace POS;
			((int*)DiskStartInRAM)[0]=1;
			((int*)DiskStartInRAM)[1]=++CurrentMark;
			((int*)DiskStartInRAM)[2]=1;
			StupidFileNode *u=new StupidFileNode(this);
			u->Attributes|=StupidFileNode::Attri_Root|StupidFileNode::Attri_Dir;
			MemcpyT<char>((char*)(DiskStartInRAM+4096),(char*)&u->ID,4096);
			delete u;
			*(int*)(DiskStartInRAM+4096*2)=CurrentMark;
			return ERR_None;
		}
		
		StupidFileNode* FindSFN(const char *path)
		{
			using namespace POS;
			ASSERTEX(path&&path[0]=='/',"StupidFileNode::FindSFN path is nullptr or path[0] is not / ! path:"<<path);
			const char *base=++path;
			StupidFileNode *u=root;
			while (u&&*path)
				if (*path=='/')
				{
					if (base!=path)
						u=u->FindChild(base,path);
					base=path=path+1;
				}
				else ++path;
			if (u!=nullptr&&base!=path)
				u=u->FindChild(base,path);
			return u;
		}
		
		virtual FileNode* FindFile(FileNode *p,const char *name)
		{
			StupidFileNode *u=(StupidFileNode*)p;
			return u->FindChild(name);
		}
		
		virtual FileNode* FindFile(const char *path,const char *name)
		{
			auto Path=POS::strSplice(path,"/",name);
			FileNode *re=FindSFN(Path);
			Kfree(Path);
			return re;
		}
		
		virtual int GetAllFileIn(FileNode *p,char *result[],int bufferSize,int skipCnt=0)
		{
			StupidFileNode *u=(StupidFileNode*)p;
			int re=0;
			if (u!=nullptr)
				for (StupidFileNode *v=u->child;v&&re<bufferSize;v=v->nxt)
					if (skipCnt==0)
						result[re++]=POS::strDump(v->Name);
					else --skipCnt;
			return re;
		}
		
		virtual int GetAllFileIn(const char *path,char *result[],int bufferSize,int skipCnt)
		{
			StupidFileNode *u=FindSFN(path);
			int re=0;
			if (u!=nullptr)
				for (StupidFileNode *v=u->child;v&&re<bufferSize;v=v->nxt)
					if (skipCnt==0)
						result[re++]=POS::strDump(v->Name);
					else --skipCnt;
			return re;
		}
		
		ErrorType CreateDirectoryFile(const char *path,int atttributes)
		{
			using namespace POS;
			ASSERTEX(path&&path[0]=='/',"StupidFileNode::CreateDirectoryFile path is nullptr or path[0] is not / ! path:"<<path);
			kout[Test]<<"StupidFileSystem::CreateDirectoryFile "<<path<<" with attribute "<<(void*)atttributes<<endl;
			ErrorType re=ERR_Unknown;
			auto dividedPath=POS::CutLastSection(path);
			StupidFileNode *dir=FindSFN(dividedPath.a);
			if (dir==nullptr||!(dir->Attributes&StupidFileNode::Attri_Dir))
				re=ERR_DirectoryPathNotExist;
			else
			{
				StupidFileNode *u=dir->FindChild(dividedPath.b);
				if (u==nullptr)
				{
					u=new StupidFileNode(this);
					u->Attributes=atttributes;
					re=u->SetFileName(dividedPath.b);
					if (re==0)
						dir->Insert(u);
					else delete u;
				}
				else re=ERR_FileAlreadyExist;
			}
			if (re)
			{
				Kfree(dividedPath.a);
				Kfree(dividedPath.b);
			}
			kout[Test]<<"StupidFileSystem::CreateDirectoryFile ErrorCode "<<re<<endl;
			return re;
		}
		
		virtual ErrorType CreateDirectory(const char *path)
		{return CreateDirectoryFile(path,StupidFileNode::Attri_Dir);}
		
		virtual ErrorType CreateFile(const char *path)
		{return CreateDirectoryFile(path,0);}
		
		virtual ErrorType Move(const char *src,const char *dst)
		{
			using namespace POS;
			kout[Fault]<<"StupidFileSystem::Move is not usable yet!"<<endl;
		}
		
		virtual ErrorType Copy(const char *src,const char *dst)
		{
			using namespace POS;
			kout[Fault]<<"StupidFileSystem::Copy is not usable yet!"<<endl;
		}
		
		virtual ErrorType Delete(const char *path)
		{
			using namespace POS;
			kout[Fault]<<"StupidFileSystem::Delete is not usable yet!"<<endl;
		}
		
		virtual FileNode* GetNextFile(const char *base)
		{
			using namespace POS;
			kout[Fault]<<"StupidFileSystem::GetNextFile is not usable yet!"<<endl;
		}
		
		virtual FileNode* Open(const char *path)
		{return FindSFN(path);}
		
		virtual ErrorType Close(FileNode *p)
		{
			using namespace POS;
			kout[Warning]<<"StupidFileSystem::Close(FileNode) perform nothing..."<<endl;
			return ERR_Todo;
		}
	
	public:
		inline ErrorType Save()
		{return SaveData();}
		
		virtual const char *FileSystemName()
		{return "StupidFileSystem";}
		
		virtual ~StupidFileSystem()
		{
			using namespace POS;
			kout[Info]<<"StupidFileSystem Deconstruct"<<endl;
			auto err=SaveData();
			ASSERTEX(err==0,"~StupidFileSystem failed to SaveData! ErrorCode "<<err);
			POS_PMM.FreePage(POS_PMM.GetPageFromAddr((void*)DiskStartInRAM));
			kout[Info]<<"StupidFileSystem Deconstruct OK"<<endl;
		}
		
		StupidFileSystem()
		{
			using namespace POS;
			kout[Info]<<"StupidFileSystem Construct"<<endl;
			DiskSize=1*1024*1024;
			DiskStartInRAM=(PtrInt)POS_PMM.AllocPage(DiskSize/PageSize)->KAddr();
			MemsetT<Uint64>((Uint64*)DiskStartInRAM,0,DiskSize/sizeof(Uint64));
			InitEmpty();
			auto err=LoadData();
			ASSERTEX(err==0,"StupidFileSystem failed to LoadData! ErrorCode "<<err);
			kout[Info]<<"StupidFileSystem Construct OK"<<endl;
		}
};

StupidFileNode::~StupidFileNode()
{
	using namespace POS;
	while (child!=nullptr)
		delete child;
	if (fa!=nullptr)
		if (fa->child==this)
		{
			fa->child=nxt;
			if (nxt!=nullptr)
				nxt->pre=nullptr;
			nxt=nullptr;
		}	
		else
		{
			pre->nxt=nxt;
			if (nxt!=nullptr)
				nxt->pre=pre;
			nxt=pre=nullptr;
		}
	fa=nullptr;
	--SFS->TotalFiles;
}

StupidFileNode::StupidFileNode(StupidFileSystem *_sfs):FileNode(_sfs,0,0),SFS(_sfs)
{
	FileNode::SetFileName(Name,1);
}

class TestMemFileNode:public FileNode
{
	protected:
		PtrInt Begin,End,Size;
		
	public:
		virtual Sint64 Read(void *dst,Uint64 pos,Uint64 size)
		{
			if (pos>=Size||pos+size>=Size)
				return ERR_FileOperationOutofRange;
			for (Uint64 i=0;i<size;++i)
				((char*)dst)[i]=((char*)Begin)[pos+i];
			return ERR_None;
		}
		
		virtual Sint64 Write(void *src,Uint64 pos,Uint64 size)
		{
			return ERR_Unknown;
		}
		
		TestMemFileNode(PtrInt begin,PtrInt end):FileNode(nullptr,0,0),Begin(begin),End(end),Size(end-begin)
		{
			using namespace POS;
			kout[Debug]<<"  This function is used for test ELF temporaryly!"<<endl;
			kout[Warning]<<"This function is used for test ELF temporaryly!"<<endl;
		}
};

#endif
