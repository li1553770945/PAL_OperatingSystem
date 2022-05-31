#ifndef POS_FAT32_HPP
#define POS_FAT32_HPP

#include "FileSystem.hpp"
#include "../HAL/Drivers/_sdcard.h"
#include <Library/TemplateTools.hpp>
#include <Library/DataStructure/PAL_Tuple.hpp>

//#undef CreateFile
//#undef CreateDirectory

class FileBaseSystem {//Test
	 ;
public:
	int Init()
	{
		// disk = CreateFileA("\\\\.\\PhysicalDrive2", GENERIC_READ , FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		// return (disk == INVALID_HANDLE_VALUE);
		return 0;
	}
	bool Read(Uint64 pos, unsigned char* buffer)
	{
		sdcard_read_sector((Sector*)buffer,pos);
		return 1;
		// memset(buffer, 0, 512);
		// LARGE_INTEGER li;
		// li.QuadPart = pos * 0x200;//0x200 = 512,求出扇区的 字节地址，通过设置读取的地址和长度进行read
		// SetFilePointer(disk, li.LowPart, &li.HighPart, FILE_BEGIN);
		// DWORD count = 0; //计数
		// BOOL result = ReadFile(disk, buffer, 512, &count, NULL);
		// return result;
	}
};
class VirtualFileSystem;

struct DBR {
	Uint32 BPB_rsvd_sec_cnt;  //保留扇区数⽬ 
	Uint32 BPB_FAT_num;   //此卷中FAT表数 
	Uint32 BPB_section_per_FAT_area;   //⼀个FAT表扇区数 
	Uint32 BPB_hiden_section_num; //隐藏扇区数
	Uint64 BPBSectionPerClus;//每个簇有多少个扇区
};

class FAT32 :public VirtualFileSystem
{
	friend class FAT32FileNode;
public:

	virtual FileNode* FindFile(const char* path, const char* name) override;
	virtual int GetAllFileIn(const char* path, char* result[], int bufferSize, int skipCnt = 0) override;//if unused,result should be empty when input , user should free the char*
	virtual ErrorType CreateDirectory(const char* path) override;
	virtual ErrorType CreateFile(const char* path) override;
	virtual ErrorType Move(const char* src, const char* dst) override;
	virtual ErrorType Copy(const char* src, const char* dst) override;
	virtual ErrorType Delete(const char* path)override;
	virtual FileNode* GetNextFile(const char* base) override;

	virtual FileNode* Open(const char* path) override;
	virtual ErrorType Close(FileNode* p) override;

	Uint32 DBRLba;
	DBR Dbr;
	Uint32 FAT1Lba;
	Uint32 FAT2Lba;
	Uint32 RootLba;//数据区(根目录)起始lba
	FileBaseSystem file;


	FAT32();
	FileNode* LoadShortFileInfoFromBuffer(unsigned char* buffer);
	ErrorType LoadLongFileNameFromBuffer(unsigned char* buffer,Uint32* name);
	char*  MergeLongNameAndToUtf8(Uint32* buffer[], Uint32 cnt);
	Uint64 GetLbaFromCluster(Uint64 cluster);
	//FileNode * GetFileNodesFromCluster(Uint64 cluster);//读取cluster开始的目录对应的所有目录项

	Uint64 GetFATContentFromCluster(Uint64 cluster);//读取cluster对应的FAT表中内容(自动将读取的内容转换为小端)
	Uint64 SetFATContentFromCluster(Uint64 cluster,Uint64 content);//设置cluster对应的FAT表中内容为content(自动将content转换为大端)
	int ReadRawData(Uint64 lba, Uint64 offset, Uint64 size, unsigned char* buffer);//从lba偏移offset字节的位置读取size字节大小的数据
	FileNode* FindFileByNameFromCluster(Uint64 cluster, const char* name);//从cluster寻找一个file，cluster对应的必须是目录项所在的位置
	FileNode* FindFileByPath(const char* path);
	bool IsExist(const char* path);
	Uint64 GetFreeCluster();//返回一个空闲簇的簇号
	PAL_DS::Doublet<Uint64,Uint64> GetContentLbaAndOffsetFromPath();//得到文件所在目录项的位置，例如要删除文件就要把文件对应目录项设置为E5
	PAL_DS::Doublet<Uint64, Uint64> GetFreeLbaAndOffsetFromPath();//得到Path中下一个空白的位置用于放置目录项


public:
	int Init();
	const char* FileSystemName() override;

	
};


class FAT32FileNode :public FileNode {
	friend class FAT32;
public:
	virtual ErrorType Read(void* dst, Uint64 pos, Uint64 size) override;
	virtual ErrorType Write(void* src, Uint64 pos, Uint64 size) override;
	PAL_DS::Doublet <Uint64, Uint64>  GetCLusterAndLba(Uint64 pos);
	Uint64 FirstCluster; //起始簇号
	FAT32FileNode* nxt;
	bool IsDir; //是否是文件夹
	Uint64 ReadSize;//已经读取的数据大小
	FAT32FileNode(FAT32* _vfs,Uint64 cluster=0);
	~FAT32FileNode();
};

#endif
