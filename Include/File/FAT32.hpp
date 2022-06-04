#ifndef POS_FAT32_HPP
#define POS_FAT32_HPP

#include "FileSystem.hpp"
#include "../HAL/Drivers/_sdcard.h"
#include <Library/TemplateTools.hpp>
#include <Library/DataStructure/PAL_Tuple.hpp>

//#undef CreateFile
//#undef CreateDirectory
const Uint64 SECTORSIZE = 512;
const Uint64 CLUSTEREND = 0x0FFFFFFF;

class StorageDevice {
	
public:
	virtual ErrorType Init() = 0;
	virtual ErrorType Read(Uint64 lba, unsigned char* buffer) = 0; 
	virtual ErrorType Write(Uint64 lba, unsigned char* buffer) = 0;
};

class FAT32Device :public StorageDevice {
public:
	ErrorType Init()
	{
		return ERR_None;;
	}
	ErrorType Read(Uint64 lba, unsigned char* buffer)
	{
//		using namespace POS;
//		kout[Debug]<<"ReadLBA "<<lba<<endl;
		sdcard_read_sector((Sector*)buffer,lba);
//		kout[Debug]<<"ReadLBA OK"<<endl;
		return ERR_None;
	}
	ErrorType Write(Uint64 lba, unsigned char* buffer)
	{
//		using namespace POS;
//		kout[Debug]<<"WriteLBA "<<lba<<endl;
		sdcard_write_sector((Sector*)buffer,lba);
//		kout[Debug]<<"WriteLBA OK"<<endl;
		return ERR_None;
	}
};
class VirtualFileSystem;

struct DBR {
	Uint32 BPB_RsvdSectorNum;  //保留扇区数⽬ 
	Uint32 BPB_FATNum;   //此卷中FAT表数 
	Uint32 BPB_SectorPerFATArea;   //⼀个FAT表扇区数 
	Uint32 BPB_HidenSectorNum; //隐藏扇区数
	Uint64 BPBSectorPerClus;//每个簇有多少个扇区
};
//class ShortContent {
//	unsigned char buffer[32];
//public:
//	void SetBuffer(unsigned char*);
//	unsigned char* GetBuffer();
//	ErrorType SetCluster(Uint32 cluster);
//	Uint32 GetCluster();
//};
class FAT32 :public VirtualFileSystem
{
	friend class FAT32FileNode;
public:

	virtual FileNode* FindFile(const char* path, const char* name) override;
	virtual int GetAllFileIn(const char* path, char* result[], int bufferSize, int skipCnt = 0) override;//if unused,result should be empty when input , user should free the char*
	virtual int GetAllFileIn(const char* path, FileNode* nodes[], int bufferSize, int skipCnt = 0) override;
	virtual ErrorType CreateDirectory(const char* path) override;
	virtual ErrorType CreateFile(const char* path) override;
	virtual ErrorType Move(const char* src, const char* dst) override;
	virtual ErrorType Copy(const char* src, const char* dst) override;
	virtual ErrorType Delete(const char* path)override;
	virtual FileNode* GetNextFile(const char* base) override;

	virtual FileNode* Open(const char* path) override;
	virtual ErrorType Close(FileNode* p) override;

	Uint64 DBRLba;
	DBR Dbr;
	Uint64 FAT1Lba;
	Uint64 FAT2Lba;
	Uint64 RootLba;//数据区(根目录)起始lba
	FAT32Device device;


	FAT32();
	FileNode* LoadShortFileInfoFromBuffer(unsigned char* buffer);
	ErrorType LoadLongFileNameFromBuffer(unsigned char* buffer,Uint32* name);
	char*  MergeLongNameAndToUtf8(Uint32* buffer[], Uint32 cnt);
	Uint64 GetLbaFromCluster(Uint64 cluster);
	Uint64 GetSectorOffsetFromlba(Uint64 lba);//当前lba是所属簇的第几个扇区

	//FileNode * GetFileNodesFromCluster(Uint64 cluster);//读取cluster开始的目录对应的所有目录项

	Uint32 GetFATContentFromCluster(Uint32 cluster);//读取cluster对应的FAT表中内容(自动将读取的内容转换为小端)
	ErrorType SetFATContentFromCluster(Uint32 cluster,Uint32 content);//设置cluster对应的FAT表中内容为content(自动将content转换为大端)
	ErrorType ReadRawData(Uint64 lba, Uint64 offset, Uint64 size, unsigned char* buffer);//从lba偏移offset字节的位置读取size字节大小的数据
	ErrorType WriteRawData(Uint64 lba, Uint64 offset, Uint64 size, unsigned char* buffer);
	FileNode* FindFileByNameFromCluster(Uint32 cluster, const char* name);//从cluster寻找一个file，cluster对应的必须是目录项所在的位置
	FileNode* FindFileByPath(const char* path);
	bool IsExist(const char* path);
	bool IsShortContent(const char* name);
	PAL_DS::Doublet <unsigned char*,Uint8 > GetShortName(const char*);
	PAL_DS::Doublet <unsigned char*, Uint64> GetLongName(const char *);
	Uint32 GetFreeClusterAndPlusOne();//返回一个空闲簇的簇号，并且把这个值加1
	ErrorType AddContentToCluster(Uint32 cluster,unsigned char * buffer,Uint64 size);
	//在cluster所在的位置找到一个空的位置，写入buffer里面的内容，大小为size，如果不够了会自动开新的簇
	PAL_DS::Doublet<Uint64,Uint64> GetContentLbaAndOffsetFromPath();//得到文件所在目录项的位置，例如要删除文件就要把文件对应目录项设置为E5
	PAL_DS::Triplet<Uint32, Uint64,Uint64>  GetFreeClusterAndLbaAndOffsetFromCluster(Uint32 cluster);//得到目录cluster中下一个空白的位置用于放置目录项
	unsigned char CheckSum(unsigned char* data);

public:
	ErrorType Init();
	const char* FileSystemName() override;

	
};


class FAT32FileNode :public FileNode {
	friend class FAT32;
public:

	Uint32 FirstCluster; //起始簇号
	FAT32FileNode* nxt;
	bool IsDir; //是否是文件夹
	Uint64 ReadSize;//已经读取的数据大小
	Uint64 ContentLba;//目录项所在lba
	Uint64 ContentOffset;//目录项所在偏移

	virtual Sint64 Read(void* dst, Uint64 pos, Uint64 size) override;
	virtual Sint64 Write(void* src, Uint64 pos, Uint64 size) override;
	ErrorType SetSize(Uint32 size);//设置文件大小，只能缩小，不能放大
	PAL_DS::Doublet <Uint32, Uint64> GetCLusterAndLbaFromOffset(Uint64 offset);
	FAT32FileNode(FAT32* _vfs,Uint32 cluster,Uint64 _ContentLba,Uint64 _ContentOffset);
	~FAT32FileNode();
};

#endif
