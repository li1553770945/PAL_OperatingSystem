#include <File/FileSystem.hpp>
#include <Library/Kout.hpp>
using namespace POS;

VirtualFileSystemManager VFSM;

ErrorType VirtualFileSystemManager::Init()
{
	kout[Warning]<<"VirtualFileSystemManager::Init is not usable!"<<endl;
	return ERR_Todo;
}
