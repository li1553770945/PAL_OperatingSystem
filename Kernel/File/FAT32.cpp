#pragma once
#include <File/FAT32.hpp>
#include <Error.hpp>
const Uint64 SECTIONSIZE = 512;
const Uint64 CLUSTEREND = 0x0FFFFFFF;


using namespace POS;

const char* FAT32::FileSystemName()
{
	return "FAT32";
}


int FAT32::ReadRawData(Uint64 lba,Uint64 offset, Uint64 size, unsigned char* buffer)
{
	unsigned char buffer_temp[SECTIONSIZE];
	ErrorType error = file.Read(lba, buffer_temp);
	if (error == 0)
	{
		return error;
	}
	POS::MemcpyT(buffer, buffer_temp + offset, size);
	return  ERR_None;
}
int FAT32::Init()
{
	kout << "initing fat32 file system..." << endl;
	if (file.Init() != 0)
	{
		return -1;
	}
	unsigned char buffer[SECTIONSIZE];
	file.Read(0, buffer);
	if (!((Uint8)buffer[510] == 0x55 && (Uint8)buffer[511] == 0xAA) )
	{
		return -1;
	}
	DBRLba = (buffer[0x1c9] << 24) | (buffer[0x1c8] << 16) | (buffer[0x1c7] << 8) | (buffer[0x1c6]);
	kout << "DBR_lba:" << DBRLba << endl;


	file.Read(DBRLba, buffer); //buffer是DBR分区内容

	Dbr.BPBSectionPerClus = buffer[0x0d];
	Dbr.BPB_rsvd_sec_cnt = (buffer[0x0f] << 8) | buffer[0x0e];
	Dbr.BPB_FAT_num = buffer[0x10];
	Dbr.BPB_hiden_section_num = (buffer[0x1f] << 24) | (buffer[0x1e] << 16) | (buffer[0x1d] << 8) | (buffer[0x1c]);
	Dbr.BPB_section_per_FAT_area = (buffer[0x27] << 24) | (buffer[0x26] << 16) | (buffer[0x25] << 8) | (buffer[0x24]); //FAT区大小

	kout  << "BPB reserved section count:" << Dbr.BPB_rsvd_sec_cnt<<endl;
	kout  << "BPB FAT number:" << Dbr.BPB_FAT_num << endl;
	kout  << "BPB hiden section num:" << Dbr.BPB_hiden_section_num << endl;
	kout  << "BPB FAT section num:" << Dbr.BPB_section_per_FAT_area << endl;
	FAT1Lba = DBRLba + Dbr.BPB_rsvd_sec_cnt; //FAT1 = DBR + 保留扇区
	FAT2Lba = FAT1Lba + Dbr.BPB_section_per_FAT_area; //FAT2 = FAT1 + FAT区大小
	RootLba = FAT1Lba + Dbr.BPB_section_per_FAT_area * Dbr.BPB_FAT_num;
	kout <<"FAT1_lba:" << FAT1Lba << " FAT2_lba:" << FAT2Lba << endl;
	kout  << "root lba:" << RootLba << endl;
	return ERR_None;
}

FileNode* FAT32::FindFile(const char* path, const char* name)
{
	return nullptr;
}
int FAT32::GetAllFileIn(const char* path, char* result[], int bufferSize, int skipCnt) 
{

	return 0;

}
ErrorType FAT32::CreateDirectory(const char* path)
{
	return 0;

}
ErrorType FAT32::CreateFile(const char* path)
{
	return 0;

}
ErrorType FAT32::Move(const char* src, const char* dst)
{
	return 0;

}
ErrorType FAT32::Copy(const char* src, const char* dst)
{
	return 0;
}
ErrorType FAT32::Delete(const char* path)
{

	return 0;
}
FileNode* FAT32::GetNextFile(const char* base)
{
	return nullptr;
}
FileNode* FAT32::Open(const char* path)
{
	return FindFileByPath(path);
}
ErrorType FAT32::Close(FileNode* p)
{
	return 0;
}
FileNode* FAT32::LoadShortFileInfoFromBuffer(unsigned char * buffer) //从第lba扇区偏移offset的位置读取文件头信息
{
	
	if (buffer[0] == 0x00 || buffer[0] == 0xE5)//如果这个位置已经被删除或没有数据则返回Null
	{
		return nullptr;
	}
	
	Uint16 attr = buffer[11];
	/*if (attr & (1 << 4))
	{
		return nullptr;
	}*/
	FAT32FileNode* node = new FAT32FileNode(this);
	node->IsDir = attr & (1 << 4);
	Uint16 file_name_length = 0; //获取文件名
	for (int i = 0; i < 8; i++)
	{
		
		if (buffer[i] == 0x20)
		{
			break;
		}
		file_name_length++;
	}

	Uint16 extend_name_length = 0;
	unsigned char* file_name;
	int total_length;
	for (int i = 0x08; i < 0x0B; i++)//读取拓展名
	{
		if (buffer[i] == 0x20)
		{
			break;
		}
		extend_name_length++;
	}
	if (!node->IsDir&&extend_name_length!=0)//不是文件夹，且有拓展名
	{
		
		total_length = file_name_length + extend_name_length + 1;
		file_name = new unsigned char[total_length + 1];
		POS::MemcpyT(file_name, buffer, file_name_length);
		file_name[file_name_length] = '.';
		POS::MemcpyT(file_name + file_name_length + 1, buffer + 0x08, extend_name_length);
	}
	else
	{
		total_length = file_name_length;
		file_name = new unsigned char[total_length + 1];
		POS::MemcpyT(file_name, buffer, file_name_length);
	}

	//1. 此值为18H时，文件名和扩展名都小写。
	//2. 此值为10H时，文件名大写而扩展名小写。
	//3. 此值为08H时，文件名小写而扩展名大写。
	//4. 此值为00H时，文件名和扩展名都大写。

	if (buffer[0x0C] == 0x08 || buffer[0x0C] == 0x18)
	{
		for (int i = 0; i < file_name_length; i++)
		{
			if (file_name[i] < 'Z' && file_name[i] > 'A')
			{
				file_name[i] += 32;
			}
		}
	}
	if (buffer[0x0C] == 0x10 || buffer[0x0C] == 0x18)
	{
		for (int i = file_name_length + 1; i <total_length; i++)
		{
			if (file_name[i] < 'Z' && file_name[i] > 'A')
			{
				file_name[i] += 32;
			}
		}
	}
	file_name[total_length] = '\0';

	node->SetFileName((char*)file_name,false);
	node->nxt = nullptr;
	node->FileSize = (buffer[0x1F] << 24) | (buffer[0x1E] << 16) | (buffer[0x1D] << 8) | (buffer[0x1C]);//获取文件大小
	node->FirstCluster = (buffer[0x15] << 24) | (buffer[0x14] << 16) | (buffer[0x1B] << 8) | (buffer[0x1A]);
	node->CurCluster = node->FirstCluster;
	//delete [] file_name; 移植的时候记得加上
	return node;

}
Uint64 FAT32::GetLbaFromCluster(Uint64 cluster)
{

	return RootLba + (cluster - 2)*Dbr.BPBSectionPerClus;

}
Uint64 FAT32::GetOffsetFromCluster(Uint64 cluster)
{
	return 0;
}

FileNode* FAT32::GetFileNodesFromCluster(Uint64 cluster) // cluster对应的是目录项
{
	FAT32FileNode* head = nullptr,* cur = nullptr;
	Uint64 lba = GetLbaFromCluster(cluster);
	for (Uint32 i = 0; i < Dbr.BPBSectionPerClus; i++)
	{
		unsigned char buffer[SECTIONSIZE];
		ReadRawData(lba+i, 0, 512, buffer);
		for (Uint32 j = 0; j < SECTIONSIZE / 32; j++)
		{
			unsigned char temp[32];
			POS::MemcpyT(temp, buffer + j * 32, 32);
			Uint16 attr = temp[11];
			if (attr == 0x0F)//长目录项
			{

			}
			else //短目录项
			{
				FAT32FileNode* p = (FAT32FileNode*)LoadShortFileInfoFromBuffer(temp);
				if (p == nullptr)
				{
					continue;
				}
				
				if (head == nullptr)
				{
					head = p;
					cur = head;
					cur->nxt = nullptr;
				}
				else
				{
					cur->nxt = (FAT32FileNode*)LoadShortFileInfoFromBuffer(temp);
					cur = cur->nxt;
					cur->nxt = nullptr;
				}
			}
		}
	}
	Uint64 nxt = GetFATContentFromCluster(cluster);
	if (nxt != CLUSTEREND)
	{

	}
	return head;
	
}
Uint64 FAT32::GetFATContentFromCluster(Uint64 cluster)
{
	Uint64 lba = FAT1Lba + cluster * 4 / SECTIONSIZE;//对应扇区lba
	Uint64 offset = (cluster * 4) % SECTIONSIZE;
	unsigned char buffer[4];
	ReadRawData(lba, offset, 4, buffer);
	return (buffer[3] << 24) | (buffer[2] << 16) | (buffer[1] <<8) | buffer[0];
}
FileNode* FAT32::FindFileByNameFromCluster(Uint64 cluster, const char* name)
{
	FAT32FileNode* result;
	Uint64 lba = GetLbaFromCluster(cluster);
	for (Uint32 i = 0; i < Dbr.BPBSectionPerClus; i++)
	{
		unsigned char buffer[SECTIONSIZE];
		ReadRawData(lba + i, 0, 512, buffer);
		for (Uint32 j = 0; j < SECTIONSIZE / 32; j++)
		{
			unsigned char temp[32];
			POS::MemcpyT(temp, buffer + j * 32, 32);
			if (temp[0] == 0x00) //遇到目录结尾
			{
				return nullptr;
			}
			Uint16 attr = temp[11];
			if (attr == 0x0F)//长目录项
			{
				result = new FAT32FileNode(this, 2);
				result->Name = new char;
				result->Name[0] = 0;
			}
			else //短目录项
			{
				result = (FAT32FileNode*)LoadShortFileInfoFromBuffer(temp);
			}
			if (result != nullptr)
			{
				if (POS::strComp(name, result->Name) == 0)
				{
					return result;
				}
			}
		}
	}
	Uint64 nxt = GetFATContentFromCluster(cluster);
	if (nxt != CLUSTEREND)
	{
		return FindFileByNameFromCluster(nxt,name);
	}
	else
	{
		return nullptr;
	}
}
FileNode* FAT32::FindFileByPath(const char* path)
{
	if (path[0] != '/')
	{
		kout << "now this system only support absolute path,which means begin with /";
		return nullptr;
	}

	if (POS::strLen(path) < 2)
	{
		return nullptr;
	}

	FAT32FileNode* node = new FAT32FileNode(this, 2);
	Uint64 index, last_index = 1;
	for (index= 1; index < POS::strLen(path); index++)
	{
		if (path[index] == '/')
		{
			if (index == last_index)
			{
				return nullptr;
			}
			char* temp = new char[(Uint32)(index - last_index + 1ull)];
			POS::MemcpyT(temp, path + last_index, index - last_index);
			temp[index - last_index] = '\0';
			kout << "find:" << temp << endl;
			FAT32FileNode* last_node = node;
			node = (FAT32FileNode*)FindFileByNameFromCluster(node->CurCluster,temp);
			if (node == nullptr)
			{
				return nullptr;
			}
			delete[] temp;
			delete last_node;
			if (index != POS::strLen(path) && node->IsDir == false)
			{
				return nullptr;
			}
			last_index = index + 1;
		}
	}
	if (last_index < POS::strLen(path)) //最后一个section
	{
		char* temp = new char[(Uint32)(POS::strLen(path)- last_index + 1)];
		POS::MemcpyT(temp, path + last_index, POS::strLen(path) - last_index);
		temp[POS::strLen(path) - last_index] = '\0';
		FAT32FileNode* last_node = node;
		node = (FAT32FileNode*)FindFileByNameFromCluster(node->CurCluster, temp);
		delete[] temp;
		delete last_node;
	}
	return node;
}
bool FAT32::IsExist(const char* path)
{
	return FindFileByPath(path) != nullptr;
}



FAT32FileNode::FAT32FileNode(FAT32* _vfs, Uint64 _cluster)
{
	IsDir = false;
	nxt = nullptr;
	Vfs = _vfs;
	FirstCluster = _cluster;
	CurCluster = _cluster;
	ReadSize = 0;
}

ErrorType FAT32FileNode::Read(void* dst, Uint64 pos, Uint64 size)
{
	FAT32* vfs = (FAT32*)Vfs;
	Uint64 total_has_read_size = 0;//可能要跨扇区、簇读取，这是这次READ多个簇读取的总数据量
	Uint64 bytes_per_cluster = SECTIONSIZE * vfs->Dbr.BPBSectionPerClus;
	if (size + ReadSize > FileSize)
	{
		return ERR_FileOperationOutofRange;
	}
	if (size == 0)
	{
		return 0;
	}

	Uint64 lba = vfs->GetLbaFromCluster(CurCluster) + (ReadSize % bytes_per_cluster) / SECTIONSIZE;//当前簇LBA

	while (size)
	{
		kout[Info] << "current lba:" << lba << endl;
		if (CurCluster == CLUSTEREND)
		{
			kout[Error] << "try to read FAT32 from cluster end"<<endl;
			return ERR_SystemError;
		}
		Uint64 section_has_read = ReadSize % (SECTIONSIZE);//当前扇区已读字节
		Uint64 section_need_read_size;//当前扇区需要读取的字节
		if (section_has_read + size <= SECTIONSIZE)//这个扇区可以直接满足
		{
			section_need_read_size = size;
			size = 0;
		}
		else//当前扇区不能满足，把这个扇区读完
		{
			section_need_read_size = SECTIONSIZE - section_has_read;
			size -= section_need_read_size;
		}
		
		vfs->ReadRawData(lba, section_has_read, section_need_read_size , (unsigned char*)dst+total_has_read_size);
		total_has_read_size += section_need_read_size;
		ReadSize += section_need_read_size;

		if (ReadSize % SECTIONSIZE == 0)//这个扇区已经读完
		{
			lba++;
		}
		if (ReadSize % bytes_per_cluster == 0) //这个簇已经读完
		{
			CurCluster = vfs->GetFATContentFromCluster(CurCluster);
			lba = vfs->GetLbaFromCluster(CurCluster);//新簇LBA
		}
	}
	
	return ERR_None;
}
ErrorType FAT32FileNode::Write(void* src, Uint64 pos, Uint64 size)
{
	return ERR_None;
}

FAT32FileNode::~FAT32FileNode()
{
	
}

