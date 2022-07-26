#ifndef POS_FAT32_HPP
#define POS_FAT32_HPP

#include "FileSystem.hpp"
//#include "../HAL/Drivers/_sdcard.h"
#include <Library/TemplateTools.hpp>
#include <Library/DataStructure/PAL_Tuple.hpp>
#include "../HAL/Disk.hpp"

//#undef CreateFile
//#undef CreateDirectory
const Uint64 SECTORSIZE = 512;
//const Uint64 CLUSTEREND = 0x0FFFFFFF;
constexpr Uint64 ClusterEndFlag=0x0FFFFFFF;

inline bool IsClusterEnd(Uint64 cluster)
{return POS::InRange(cluster,0x0FFFFFF8,0x0FFFFFFF);}

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
	ErrorType Read(Uint64 lba, unsigned char* buffer)//Need improve: replace buffer with Sector
	{
		CallingStackController csc("DeviceRead");
//		using namespace POS;
//		kout[Debug]<<"ReadLBA "<<lba<<endl;
//		sdcard_read_sector((Sector*)buffer,lba);
		DiskReadSector(lba,(Sector*)buffer);
//		kout[Debug]<<"ReadLBA OK"<<endl;
		return ERR_None;
	}
	ErrorType Write(Uint64 lba, unsigned char* buffer)
	{
//		using namespace POS;
//		kout[Debug]<<"WriteLBA "<<lba<<endl;
//		sdcard_write_sector((Sector*)buffer,lba);
		DiskReadSector(lba,(Sector*)buffer);
//		kout[Debug]<<"WriteLBA OK"<<endl;
		return ERR_None;
	}
};
class VirtualFileSystem;

struct DBR {
	Uint32 BPB_RsvdSectorNum;  //����������? 
	Uint32 BPB_FATNum;   //�˾���FAT���� 
	Uint32 BPB_SectorPerFATArea;   //?��FAT�������� 
	Uint32 BPB_HidenSectorNum; //����������
	Uint64 BPBSectorPerClus;//ÿ�����ж��ٸ�����
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
	Uint64 RootLba;//������(��Ŀ¼)��ʼlba
	FAT32Device device;


	FAT32();
	FileNode* LoadShortFileInfoFromBuffer(unsigned char* buffer);
	ErrorType LoadLongFileNameFromBuffer(unsigned char* buffer,Uint32* name);
	char*  MergeLongNameAndToUtf8(Uint32* buffer[], Uint32 cnt);
	Uint64 GetLbaFromCluster(Uint64 cluster);
	Uint64 GetSectorOffsetFromlba(Uint64 lba);//��ǰlba�������صĵڼ�������

	//FileNode * GetFileNodesFromCluster(Uint64 cluster);//��ȡcluster��ʼ��Ŀ¼��Ӧ������Ŀ¼��

	Uint32 GetFATContentFromCluster(Uint32 cluster);//��ȡcluster��Ӧ��FAT��������(�Զ�����ȡ������ת��ΪС��)
	ErrorType SetFATContentFromCluster(Uint32 cluster,Uint32 content);//����cluster��Ӧ��FAT��������Ϊcontent(�Զ���contentת��Ϊ���)
	ErrorType ReadRawData(Uint64 lba, Uint64 offset, Uint64 size, unsigned char* buffer);//��lbaƫ��offset�ֽڵ�λ�ö�ȡsize�ֽڴ�С������
	ErrorType WriteRawData(Uint64 lba, Uint64 offset, Uint64 size, unsigned char* buffer);
	FileNode* FindFileByNameFromCluster(Uint32 cluster, const char* name);//��clusterѰ��һ��file��cluster��Ӧ�ı�����Ŀ¼�����ڵ�λ��
	FileNode* FindFileByPath(const char* path);
	bool IsExist(const char* path);
	bool IsShortContent(const char* name);
	PAL_DS::Doublet <unsigned char*,Uint8 > GetShortName(const char*);
	PAL_DS::Doublet <unsigned char*, Uint64> GetLongName(const char *);
	Uint32 GetFreeClusterAndPlusOne();//����һ�����дصĴغţ����Ұ����ֵ��1
	ErrorType AddContentToCluster(Uint32 cluster,unsigned char * buffer,Uint64 size);
	//��cluster���ڵ�λ���ҵ�һ���յ�λ�ã�д��buffer��������ݣ���СΪsize����������˻��Զ����µĴ�
	PAL_DS::Doublet<Uint64,Uint64> GetContentLbaAndOffsetFromPath();//�õ��ļ�����Ŀ¼���λ�ã�����Ҫɾ���ļ���Ҫ���ļ���ӦĿ¼������ΪE5
	PAL_DS::Triplet<Uint32, Uint64,Uint64>  GetFreeClusterAndLbaAndOffsetFromCluster(Uint32 cluster);//�õ�Ŀ¼cluster����һ���հ׵�λ�����ڷ���Ŀ¼��
	unsigned char CheckSum(unsigned char* data);

public:
	ErrorType Init();
	const char* FileSystemName() override;

	
};


class FAT32FileNode :public FileNode {
	friend class FAT32;
public:

	Uint32 FirstCluster; //��ʼ�غ�
	FAT32FileNode* nxt;
	bool IsDir; //�Ƿ����ļ���
	Uint64 ReadSize;//�Ѿ���ȡ�����ݴ�С
	Uint64 ContentLba;//Ŀ¼������lba
	Uint64 ContentOffset;//Ŀ¼������ƫ��

	virtual Sint64 Read(void* dst, Uint64 pos, Uint64 size) override;
	virtual Sint64 Write(void* src, Uint64 pos, Uint64 size) override;
	ErrorType SetSize(Uint32 size);//�����ļ���С��ֻ����С�����ܷŴ�
	PAL_DS::Doublet <Uint32, Uint64> GetCLusterAndLbaFromOffset(Uint64 offset);
	FAT32FileNode(FAT32* _vfs,Uint32 cluster,Uint64 _ContentLba,Uint64 _ContentOffset);
	~FAT32FileNode();
};

#endif
