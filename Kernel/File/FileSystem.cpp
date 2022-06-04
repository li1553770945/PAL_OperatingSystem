#include <File/FileSystem.hpp>
#include <File/FileNodeEX.hpp>
#include <Library/Kout.hpp>
#include <Process/Process.hpp>
using namespace POS;

VirtualFileSystemManager VFSM;
UartFileNode *stdIO=nullptr;

void VirtualFileSystemManager::AddNewNode(FileNode *p,FileNode *fa)
{
	p->SetFa(fa);
	//Do something else?
}

FileNode* VirtualFileSystemManager::AddFileInVFS(FileNode *p,char *name)
{
	FileNode *re=nullptr;
	if (p->Flags&FileNode::F_BelongVFS)
		re=p->Vfs->FindFile(p,name);
	if (re)
		AddNewNode(re,p);
	return re;
}

FileNode* VirtualFileSystemManager::FindChildName(FileNode *p,const char *s,const char *e)
{
	if (s==nullptr||s>=e) return nullptr;
	for (FileNode *u=p->child;u;u=u->nxt)
		if (strComp(s,e,u->Name)==0)
			return u;
	return nullptr;
}

FileNode* VirtualFileSystemManager::FindChildName(FileNode *p,const char *s)
{
	if (s==nullptr) return nullptr;
	for (FileNode *u=p->child;u;u=u->nxt)
		if (strComp(s,u->Name)==0)
			return u;
	return nullptr;
}

FileNode* VirtualFileSystemManager::FindRecursive(FileNode *p,const char *path)
{
	if (*path==0)
		return p;
	const char *s=path+1;
	while (NotInSet(*s,0,'/'))
		++s;
	if (s==path+1)
		return p;
	char *name=strDump(path+1,s);
	FileNode *child=FindChildName(p,name);
	if (child==nullptr&&(p->Flags&FileNode::F_BelongVFS))
		child=AddFileInVFS(p,name);
	Kfree(name);
	if (child!=nullptr)
		return FindRecursive(child,s);
	else return nullptr;
}

PAL_DS::Doublet <VirtualFileSystem*,const char*> VirtualFileSystemManager::FindPathOfVFS(FileNode *p,const char *path)
{
	if (*path==0)
		return {nullptr,nullptr};
	const char *s=path+1;
	while (NotInSet(*s,0,'/'))
		++s;
	if (s==path+1)
		return {nullptr,nullptr};
	char *name=strDump(path+1,s);
	FileNode *child=FindChildName(p,name);
	if (child==nullptr)
		return {nullptr,nullptr};
	else if (child->Attributes&FileNode::A_VFS)
	{
		Kfree(name);
		return {child->Vfs,s};
	}
	else
	{
		auto re=FindPathOfVFS(child,s);
		Kfree(name);
		return re;
	}
}

char* VirtualFileSystemManager::NormalizePath(const char *path,const char *base)
{
	char *tmp=base==nullptr||IsAbsolutePath(path)?strDump(path):strSplice(base,"/",path);
	ASSERTEX(*tmp=='/',"VirtualFileSystemManager::NormalizePath "<<tmp<<" is not regular!");
	char *s=tmp,*p=tmp+1,*q=tmp+1;
	while (1)
		if (InThisSet(*q,0,'/'))
		{
			if (p==q||q==p+1&&*p=='.')
				DoNothing;
			else if (q==p+2&&*p=='.'&&*(p+1)=='.')
				while (s>tmp&&*--s!='/');
			else
				while (p<=q)
					*++s=*p++;
			if (*q==0)
				break;
			else p=q=q+1;
		}
		else ++q;
	*s=0;
	char *re=strDump(tmp);
	Kfree(tmp);
	return re;
}

FileNode* VirtualFileSystemManager::FindFile(const char *path,const char *name)
{
	kout[Warning]<<"VirtualFileSystemManager::FindFile is not usable yet!"<<endl;
	return nullptr;
}

int VirtualFileSystemManager::GetAllFileIn(const char *path,char *result[],int bufferSize,int skipCnt)
{
//	kout[Debug]<<"G3"<<endl;
	FileNode *p=FindRecursive(root,path);
//	kout[Debug]<<"G4"<<endl;
	if (p==nullptr)
		return 0;
//	kout[Debug]<<"G5"<<endl;
	if (p->Flags&FileNode::F_BelongVFS)
		return p->Vfs->GetAllFileIn(p,result,bufferSize,skipCnt);
//	kout[Debug]<<"G6"<<endl;
	int re=0;
	for (FileNode *u=p->child;u&&re<bufferSize;u=u->nxt)
		if (skipCnt)
			--skipCnt;
		else result[re++]=strDump(u->Name);
//	kout[Debug]<<"G7"<<endl;
	return re;
}

int VirtualFileSystemManager::GetAllFileIn(Process *proc,const char *path,char *result[],int bufferSize,int skipCnt)
{
//	kout[Debug]<<"G1"<<endl;
	char *pa=NormalizePath(path,proc->GetCWD());
//	kout[Debug]<<"G3"<<endl;
	int re=GetAllFileIn(pa,result,bufferSize,skipCnt);
//	kout[Debug]<<"G8"<<endl;
	Kfree(pa);
	return re;
}

ErrorType VirtualFileSystemManager::CreateDirectory(const char *path)//Need improve...
{
	auto vfs=FindPathOfVFS(root,path);
	return vfs.a->CreateDirectory(vfs.b);
}

ErrorType VirtualFileSystemManager::CreateDirectory(Process *proc,const char *path)
{
	char *pa=NormalizePath(path,proc->GetCWD());
	ErrorType re=CreateDirectory(pa);
	Kfree(pa);
	return re;
}

ErrorType VirtualFileSystemManager::CreateFile(const char *path)
{
	auto vfs=FindPathOfVFS(root,path);
	return vfs.a->CreateFile(vfs.b);
}

ErrorType VirtualFileSystemManager::CreateFile(Process *proc,const char *path)
{
	char *pa=NormalizePath(path,proc->GetCWD());
	ErrorType re=CreateFile(pa);
	Kfree(pa);
	return ERR_None;
}

ErrorType VirtualFileSystemManager::Move(const char *src,const char *dst)
{
	kout[Warning]<<"VirtualFileSystemManager::Move is not usable yet!"<<endl;
	return ERR_Todo;
}

ErrorType VirtualFileSystemManager::Copy(const char *src,const char *dst)
{
	kout[Warning]<<"VirtualFileSystemManager::Copy is not usable yet!"<<endl;
	return ERR_Todo;
}

ErrorType VirtualFileSystemManager::Delete(const char *path)
{
	kout[Warning]<<"VirtualFileSystemManager::Delete is not usable yet!"<<endl;
	return ERR_Todo;
}

ErrorType VirtualFileSystemManager::LoadVFS(VirtualFileSystem *vfs,const char *path)
{
	FileNode *p=FindRecursive(root,path);
	if (p==nullptr)
		return ERR_DirectoryPathNotExist;
	FileNode *v=new FileNode(vfs,FileNode::A_Dir|FileNode::A_VFS,FileNode::F_BelongVFS|FileNode::F_Managed|FileNode::F_Base);
	v->SetFileName((char*)vfs->FileSystemName(),0);
	AddNewNode(v,p);
	return ERR_None;
}

FileNode* VirtualFileSystemManager::Open(const char *path)
{
	return FindRecursive(root,path);
}

FileNode* VirtualFileSystemManager::Open(Process *proc,const char *path)
{
	char *pa=NormalizePath(path,proc->GetCWD());
	FileNode *re=Open(pa);
	Kfree(pa);
	return re;
}

ErrorType VirtualFileSystemManager::Close(FileNode *p)
{
	kout[Warning]<<"VirtualFileSystemManager::Close "<<p<<" is not usable yet!"<<endl;
	return ERR_Todo;
}

ErrorType VirtualFileSystemManager::Init()
{
	kout[Warning]<<"VirtualFileSystemManager::Init is not usable!"<<endl;
	
	root=new FileNode(nullptr,FileNode::A_Root|FileNode::A_Dir,FileNode::F_Managed|FileNode::F_Base);

	FileNode *Dir_VFS=new FileNode(nullptr,FileNode::A_Dir,FileNode::F_Managed|FileNode::F_Base);
	Dir_VFS->SetFileName("VFS",1);
	AddNewNode(Dir_VFS,root);

	FileNode *Dir_Dev=new FileNode(nullptr,FileNode::A_Dir,FileNode::F_Managed|FileNode::F_Base);
	Dir_Dev->SetFileName("Dev",1);
	AddNewNode(Dir_Dev,root);

	stdIO=new UartFileNode();//??
	AddNewNode(stdIO,Dir_Dev);
	return ERR_Todo;
}

ErrorType VirtualFileSystemManager::Destroy()
{
	kout[Warning]<<"VirtualFileSystemManager::Destroy is uncompleted yet and not needed yet."<<endl;
	return ERR_Todo;
}
