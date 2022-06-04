#pragma once
#include <File/FAT32.hpp>
#include <Error.hpp>
#include <Library/TemplateTools.hpp>
#include <Library/String/StringTools.hpp>
#include <Library/DataStructure/PAL_Tuple.hpp>

using namespace POS;

const char* FAT32::FileSystemName()
{
	return "FAT32";
}

FAT32::FAT32()
{
	Init();
}

ErrorType FAT32::ReadRawData(Uint64 lba,Uint64 offset, Uint64 size, unsigned char* buffer)
{
	unsigned char buffer_temp[SECTORSIZE];
	ErrorType error = device.Read(lba, buffer_temp);
	if (error != 0)
	{
		return ERR_DeviceReadError;
	}
	POS::MemcpyT(buffer, buffer_temp + offset, (Uint32)size);
	return  ERR_None;
}
ErrorType FAT32::WriteRawData(Uint64 lba, Uint64 offset, Uint64 size, unsigned char* buffer)
{
	unsigned char temp_buffer[512];
	device.Read(lba, temp_buffer);
	POS::MemcpyT(temp_buffer + offset, buffer, size);
	if (device.Write(lba, temp_buffer) == 0)
	{
		return ERR_DeviceWriteError;
	}
	return ERR_None;
}
ErrorType FAT32::Init()
{
	kout << "initing fat32 file system..." << endl;
	ErrorType err = device.Init();
	if (err != 0)
	{
		kout[Fault] << "device init error:" << err << endl;
		return err;
	}
	unsigned char buffer[SECTORSIZE];
	device.Read(0, buffer);
	if (!((Uint8)buffer[510] == 0x55 && (Uint8)buffer[511] == 0xAA) )
	{
		return ERR_VertifyNumberDisagree;
	}
	DBRLba = (buffer[0x1c9] << 24) | (buffer[0x1c8] << 16) | (buffer[0x1c7] << 8) | (buffer[0x1c6]);
	kout << "DBR_lba:" << DBRLba << endl;


	device.Read(DBRLba, buffer); //buffer是DBR分区内容

	Dbr.BPBSectorPerClus = buffer[0x0d];
	Dbr.BPB_RsvdSectorNum = (buffer[0x0f] << 8) | buffer[0x0e];
	Dbr.BPB_FATNum = buffer[0x10];
	Dbr.BPB_HidenSectorNum = (buffer[0x1f] << 24) | (buffer[0x1e] << 16) | (buffer[0x1d] << 8) | (buffer[0x1c]);
	Dbr.BPB_SectorPerFATArea = (buffer[0x27] << 24) | (buffer[0x26] << 16) | (buffer[0x25] << 8) | (buffer[0x24]); //FAT区大小
	kout << "Dbr.BPBSectionPerClus:" << Dbr.BPBSectorPerClus << endl;
	kout << "BPB reserved sector count:" << Dbr.BPB_RsvdSectorNum<<endl;
	kout << "BPB FAT number:" << Dbr.BPB_FATNum << endl;
	kout << "BPB hiden sector num:" << Dbr.BPB_HidenSectorNum << endl;
	kout << "BPB FAT sector num:" << Dbr.BPB_SectorPerFATArea << endl;
	FAT1Lba = DBRLba + Dbr.BPB_RsvdSectorNum; //FAT1 = DBR + 保留扇区
	FAT2Lba = FAT1Lba + Dbr.BPB_SectorPerFATArea; //FAT2 = FAT1 + FAT区大小
	RootLba = FAT1Lba + Dbr.BPB_SectorPerFATArea * (Uint64)Dbr.BPB_FATNum;
	kout <<"FAT1_lba:" << FAT1Lba << " FAT2_lba:" << FAT2Lba << endl;
	kout << "root lba:" << RootLba << endl;
	return ERR_None;
}

FileNode* FAT32::FindFile(const char* path, const char* name)
{
	char* s = strComp(path, "/") == 0 ? strSplice("/", name) : strSplice(path, "/", name);
	FileNode* re = Open(s);
	Kfree(s);
	return re;
}
int FAT32::GetAllFileIn(const char* path, char* result[], int bufferSize, int skipCnt) 
{
	FAT32FileNode* head = nullptr, * cur = nullptr;
	FAT32FileNode* node = (FAT32FileNode*)FindFileByPath(path);
	if (node == nullptr || !node->IsDir)
	{
		return -1;
	}
	Uint32  cluster = node->FirstCluster;
	delete node;
	int cnt = 0; //一共找到了几个文件（夹）
	int long_name_cnt = 0;//长文件名计数
	Uint32 * long_name [100];//存储长文件名

	while (cluster != CLUSTEREND)
	{
		Uint64 lba = GetLbaFromCluster(cluster);

		for (Uint32 i = 0; i < Dbr.BPBSectorPerClus; i++)
		{
			unsigned char buffer[SECTORSIZE];
			ReadRawData(lba + i, 0, 512, buffer);
			for (Uint32 j = 0; j < SECTORSIZE / 32; j++)
			{
				unsigned char temp[32];
				POS::MemcpyT(temp, buffer + j * 32, 32);
				Uint16 attr = temp[11];
				if (attr == 0x0F && temp[0] != 0xE5 && temp[0] != 0x00)//长目录项
				{
				    Uint32 * temp_long_name = new Uint32[13]; //has delete
					LoadLongFileNameFromBuffer(temp, temp_long_name);
					long_name[long_name_cnt] = temp_long_name;
					long_name_cnt++;
				}
				else //短目录项
				{
					FAT32FileNode* p = (FAT32FileNode*)LoadShortFileInfoFromBuffer(temp);
					
					if (p != nullptr)
					{
						p->ContentLba = lba + i;
						p->ContentOffset = j * 32;
						if (skipCnt)
						{
							skipCnt--;
							if (long_name_cnt)
							{
								for (int i = 0; i < long_name_cnt; i++)
								{
									delete[] long_name[i];
								}
								long_name_cnt = 0;
							}

						}
						else
						{
							char* full_name;
							if (long_name_cnt) //如果是长目录对应的短目录
							{

								full_name = MergeLongNameAndToUtf8(long_name, long_name_cnt);
								if (long_name_cnt)
								{
									for (int i = 0; i < long_name_cnt; i++)
									{
										delete[] long_name[i];
									}
									long_name_cnt = 0;
								}
							}
							else
							{
								full_name = new char[15]; //don't need delete
								POS::strCopy(full_name, p->Name);

							}

							result[cnt] = full_name;


							cnt++;
							if (cnt == bufferSize)
							{
								delete p;
								return cnt;
							}
						}
						
						
						delete p;
					}
					
					
				}
			}
		}
		cluster = GetFATContentFromCluster(cluster);
	}
	return cnt;
}


int FAT32::GetAllFileIn(const char* path, FileNode* nodes[], int bufferSize, int skipCnt)
{
	if (bufferSize <= 0)
	{
		return 0;
	}

	FAT32FileNode* node = (FAT32FileNode*)FindFileByPath(path);
	if (node == nullptr || !node->IsDir)
	{
		return -1;
	}
	Uint32  cluster = node->FirstCluster;
	delete node;
	int cnt = 0; //一共找到了几个文件（夹）
	int long_name_cnt = 0;//长文件名计数
	Uint32* long_name[100];//存储长文件名

	while (cluster != CLUSTEREND)
	{
		Uint64 lba = GetLbaFromCluster(cluster);

		for (Uint32 i = 0; i < Dbr.BPBSectorPerClus; i++)
		{
			unsigned char buffer[SECTORSIZE];
			ReadRawData(lba + i, 0, 512, buffer);
			for (Uint32 j = 0; j < SECTORSIZE / 32; j++)
			{
				unsigned char temp[32];
				POS::MemcpyT(temp, buffer + j * 32, 32);
				Uint16 attr = temp[11];
				if (attr == 0x0F && temp[0] != 0xE5 && temp[0] != 0x00)//长目录项
				{
					Uint32* temp_long_name = new Uint32[13]; //has delete
					LoadLongFileNameFromBuffer(temp, temp_long_name);
					long_name[long_name_cnt] = temp_long_name;
					long_name_cnt++;
				}
				else //短目录项
				{
					FAT32FileNode* p = (FAT32FileNode*)LoadShortFileInfoFromBuffer(temp);

					if (p != nullptr)
					{
						p->ContentLba = lba + i;
						p->ContentOffset = j * 32;
						if (skipCnt)
						{
							skipCnt--;
							if (long_name_cnt)
							{
								for (int i = 0; i < long_name_cnt; i++)
								{
									delete[] long_name[i];
								}
								long_name_cnt = 0;
							}

						}
						else
						{

							if (long_name_cnt) //如果是长目录对应的短目录
							{
								char* full_name;
								full_name = MergeLongNameAndToUtf8(long_name, long_name_cnt);
								for (int i = 0; i < long_name_cnt; i++)
								{
									delete[] long_name[i];
								}
								long_name_cnt = 0;
								p->SetFileName(full_name, false);
								delete[] full_name;
							}

							nodes[cnt] = p;
							cnt++;
							if (cnt == bufferSize)
							{
								return cnt;
							}
						}

					}


				}
			}
		}
		cluster = GetFATContentFromCluster(cluster);
	}
	return cnt;
}

ErrorType FAT32::CreateDirectory(const char* path)
{
	FAT32FileNode* node = (FAT32FileNode*)FindFileByPath(path);
	if (node != nullptr)
	{
		return ERR_FileAlreadyExist;
	}
	PAL_DS::Doublet <char*, char*> sections = CutLastSection(path);
	char* section1 = sections.a,* section2 = sections.b;
	node = (FAT32FileNode*)FindFileByPath(section1);
	if (node == nullptr)
	{
		return ERR_FilePathNotExist;
	}
	if (node->IsDir == false)
	{
		return ERR_PathIsNotDirectory;
	}

	if (IsShortContent(section2))
	{
		unsigned char* buffer = new unsigned char[32];

		PAL_DS::Doublet <unsigned char*, Uint8 > short_name = GetShortName(section2);

		MemcpyT(buffer, short_name.a, 11);
		delete[] short_name.a;
		buffer[0x0C] = short_name.b;

		Uint32 cluster = GetFreeClusterAndPlusOne();

		buffer[0x14] = (cluster & 0x00FF0000) >> 16;//设置文件簇号
		buffer[0x15] = (cluster & 0xFF000000) >> 24;
		buffer[0x1A] = cluster & 0x000000FF;
		buffer[0x1B] = (cluster & 0x0000FF00) >> 8;
		buffer[0x0B] = 1 << 4;
	
		buffer[0x0D] = 0xB6;//创建时间10ms的值
		buffer[0x0E] = 0x05;//创建时间
		buffer[0x0F] = 0x7A;

		buffer[0x10] = 0xC1;//创建日期
		buffer[0x11] = 0x54;

		buffer[0x12] = 0xC1;//最后访问日期
		buffer[0x13] = 0x54;


		buffer[0x16] = 0x06;//修改时间
		buffer[0x17] = 0x7A;

		buffer[0x18] = 0xC1;//修改日期
		buffer[0x19] = 0x54;

		AddContentToCluster(node->FirstCluster, buffer, 32ull);

		delete node;
		delete[] buffer;
		delete[] section1;
		delete[] section2;
		return ERR_None;

	}
	else
	{
		PAL_DS::Doublet <unsigned char*, Uint64>  long_name = GetLongName(section2);

		unsigned char* buffer = long_name.a;
		Uint64 total_content_num = long_name.b;
		unsigned char* temp = buffer + (total_content_num - 1) * 32;
		Uint32 cluster = GetFreeClusterAndPlusOne();

		temp[0x14] = (cluster & 0x00FF0000) >> 16;//设置文件簇号
		temp[0x15] = (cluster & 0xFF000000) >> 24;
		temp[0x1A] = cluster & 0x000000FF;
		temp[0x1B] = (cluster & 0x0000FF00) >> 8;
		temp[0x0B] = 1 << 4;

		temp[0x0D] = 0xB6;//创建时间10ms的值

		temp[0x0E] = 0x05;//创建时间
		temp[0x0F] = 0x7A;

		temp[0x10] = 0xC1;//创建日期
		temp[0x11] = 0x54;

		temp[0x12] = 0xC1;//最后访问日期
		temp[0x13] = 0x54;

		temp[0x16] = 0x06;//修改时间
		temp[0x17] = 0x7A;

		temp[0x18] = 0xC1;//修改日期
		temp[0x19] = 0x54;

		AddContentToCluster(node->FirstCluster, buffer, total_content_num * 32ull);

		delete node;
		delete[] buffer;
		delete[] section1;
		delete[] section2;
		return ERR_None;
	}

}
bool FAT32::IsShortContent(const char* name)
{
	int ext_start = -1;
	bool name_upper_case = IsUpperCase(name[0]);
	bool ext_upper_case = IsUpperCase(name[strLen(name)-1]);


	for (int i = 0; i < strLen(name); i++)
	{
		if (name[i] == '.')
		{
			if (ext_start != -1)
			{
				return false;
			}
			ext_start = i+1;
		}
	}
	if (ext_start!=-1&&ext_start - 1 > 8)
	{
		return false;
	}
	if (ext_start != -1&&strLen(name) - ext_start  > 3)
	{
		return false;
	}

	bool ext = false;
	for (int i = 0; i < strLen(name); i++)
	{
		if (name[i] == '.')
		{
			ext = true;
			continue;
		}

		if (!IsLetter(name[i]))
		{
			return false;
		}

		if (ext)
		{
			if (IsUpperCase(name[i]) != ext_upper_case)
			{
				return false;
			}
		}
		else
		{
			if (IsUpperCase(name[i]) != name_upper_case)
			{
				return false;
			}
		}
		
	}
	return true;
}
PAL_DS::Doublet <unsigned char*, Uint8 > FAT32::GetShortName(const char* name)
{
	bool have_ext = false;
	bool name_upper_case = IsUpperCase(name[0]);
	bool ext_upper_case = IsUpperCase(name[strLen(name) - 1]);


	
	unsigned char* buffer = new unsigned char [11];
	int read_pos = 0,write_pos = -1;
	for (; read_pos < strLen(name); read_pos++)
	{
		if (name[read_pos] == '.')
		{
			break;
		}
		buffer[++write_pos] = name[read_pos];
	}
	write_pos++;
	for (; write_pos < 8; write_pos++)
	{
		buffer[write_pos] = 0x20;
	}

	write_pos = 7;

	read_pos++;

	for (; read_pos < strLen(name); read_pos++)
	{
		have_ext = true;
		buffer[++write_pos] = name[read_pos];
	}
	write_pos++;

	for (; write_pos < 11; write_pos++)
	{
		buffer[write_pos] = 0x20;
	}

	for (int i = 0; i < 11; i++)
	{
		if (IsLowerCase(buffer[i]))
		{
			buffer[i] -= 32;
		}
	}
	//1. 此值为18H时，文件名和扩展名都小写。
	//2. 此值为10H时，文件名大写而扩展名小写。
	//3. 此值为08H时，文件名小写而扩展名大写。
	//4. 此值为00H时，文件名和扩展名都大写。
	if (have_ext)
	{
		if (!name_upper_case && !ext_upper_case)
		{
			return Doublet <unsigned char*, Uint8 >(buffer, 0x18);
		}
		else if (name_upper_case && !ext_upper_case)
		{
			return Doublet <unsigned char*, Uint8 >(buffer, 0x10);
		}
		else if (!name_upper_case && ext_upper_case)
		{
			return Doublet <unsigned char*, Uint8 >(buffer, 0x08);
		}
		else 
		{
			return Doublet <unsigned char*, Uint8 >(buffer, 0x00);
		}
	}
	else
	{
		if (name_upper_case)
		{
			return Doublet <unsigned char*, Uint8 >(buffer, 0x00);
		}
		else
		{
			return Doublet <unsigned char*, Uint8 >(buffer, 0x18);
		}
	}
}
PAL_DS::Doublet <unsigned char*, Uint64> FAT32::GetLongName(const char *name)
{
	PAL_DS::Doublet<Uint32*, Uint32> result = Utf8ToUnicode(name);
	Uint32* unicode = result.a;
	int len = result.b;
	int read_pos = -1;
	int total_content_num = len / 13 + 2;
	unsigned char* buffer = new unsigned char[total_content_num*32];
	MemsetT(buffer, (unsigned char)0, total_content_num * 32);
	
	for (int i = total_content_num - 1; i > 1; i--)
	{
		unsigned char* temp = buffer + (i-1)  * 32;
		temp[0x00] = total_content_num - i;
		temp[0x0B] = 0x0F;
		for (int j = 0x01; j <= 0x1F; j+=2)
		{
			if (j == 0x0B)
			{
				j = 0x0E;
			}
			if (j == 0x1A)
			{
				j = 0x1C;
			}
			temp[j]   = unicode[++read_pos] & 0x000000FF;
			temp[j+1] = (unicode[read_pos] & 0x0000FF00) >> 8;
		}
		

	}

	buffer[0x00] = 0x40 | (total_content_num - 1);
	buffer[0x0B] = 0x0F;
	bool have_add_zero = false;
	for (int i = 0x01; i <= 0x1F; i+=2)
	{
		if (i == 0x0B)
		{
			i = 0x0E;
		}
		if (i == 0x1A)
		{
			i = 0x1C;
		}
		if (read_pos + 1 >= len)
		{
			if (have_add_zero)
			{
				buffer[i] = 0xFF;
				buffer[i + 1] = 0xFF;
			}
			else
			{
				buffer[i] = 0x00;
				buffer[i + 1] = 0x00;
				have_add_zero = true;
			}
		}
		else
		{
			buffer[i] = unicode[++read_pos];
			buffer[i + 1] = (unicode[read_pos] & 0x0000FF00) >> 8;
		}
	}
	unsigned char* temp = buffer + (total_content_num - 1) * 32;


	int short_name_len = 0;
	for (int i = 0; i < 6; i++)
	{
		if (i < strLen(name))
		{
			short_name_len++;
			temp[i] = name[i];
		}
		else
		{
			break;
		}
	}
	temp[short_name_len++]  = '~';
	temp[short_name_len++] = '1';
	for (; short_name_len < 11; short_name_len++)
	{
		temp[short_name_len] = 0x20;
	}
	for (int i = 0; i < 11; i++)
	{
		if (temp[i] <= 'z' && temp[i] >= 'a')
		{
			temp[i] -= 32;
		}
	}

	unsigned char check_sum = CheckSum(temp);
	for (int i = 0; i < total_content_num - 1; i++)
	{
		buffer[i*32+0x0D] = check_sum;

	}

	delete[] unicode;
	return PAL_DS::Doublet <unsigned char*, Uint64> (buffer, total_content_num);
}
ErrorType FAT32::CreateFile(const char* path)
{
	FAT32FileNode* node = (FAT32FileNode*)FindFileByPath(path);
	if (node != nullptr)
	{
		return ERR_FileAlreadyExist;
	}
	PAL_DS::Doublet <char*, char*> sections = CutLastSection(path);
	char* section1 = sections.a, * section2 = sections.b;
	node = (FAT32FileNode*)FindFileByPath(section1);
	if (node == nullptr)
	{
		return ERR_FilePathNotExist;
	}
	if (node->IsDir == false)
	{
		return ERR_PathIsNotDirectory;
	}

	if (IsShortContent(section2))
	{
		unsigned char* buffer = new unsigned char[32];
		MemsetT(buffer, (unsigned char)0, 32);
		
		PAL_DS::Doublet <unsigned char*, Uint8 > short_name = GetShortName(section2);
		
		MemcpyT(buffer, short_name.a, 11);
		delete[] short_name.a;
		buffer[0x0C] = short_name.b;

		Uint32 cluster = GetFreeClusterAndPlusOne();

		buffer[0x14] = (cluster & 0x00FF0000) >> 16;//设置文件簇号
		buffer[0x15] = (cluster & 0xFF000000) >> 24;
		buffer[0x1A] = cluster & 0x000000FF;
		buffer[0x1B] = (cluster & 0x0000FF00) >> 8;

		buffer[0x0D] = 0xB6;//创建时间10ms的值
		buffer[0x0E] = 0x05;//创建时间
		buffer[0x0F] = 0x7A;

		buffer[0x10] = 0xC1;//创建日期
		buffer[0x11] = 0x54;

		buffer[0x12] = 0xC1;//最后访问日期
		buffer[0x13] = 0x54;


		buffer[0x16] = 0x06;//修改时间
		buffer[0x17] = 0x7A;

		buffer[0x18] = 0xC1;//修改日期
		buffer[0x19] = 0x54;

		AddContentToCluster(node->FirstCluster, buffer, 32ull);

		delete node;
		delete[] buffer;
		delete[] section1;
		delete[] section2;
		return ERR_None;

	}
	else
	{
		
		PAL_DS::Doublet <unsigned char*, Uint64>  long_name = GetLongName(section2);

		unsigned char* buffer = long_name.a;
		Uint64 total_content_num = long_name.b;
		unsigned char* temp = buffer + (total_content_num - 1)*32;
		Uint32 cluster = GetFreeClusterAndPlusOne();

		temp[0x14] = (cluster & 0x00FF0000) >> 16;//设置文件簇号
		temp[0x15] = (cluster & 0xFF000000) >> 24;
		temp[0x1A] = cluster & 0x000000FF;
		temp[0x1B] = (cluster & 0x0000FF00) >> 8;

		temp[0x0D] = 0xB6;//创建时间10ms的值

		temp[0x0E] = 0x05;//创建时间
		temp[0x0F] = 0x7A;

		temp[0x10] = 0xC1;//创建日期
		temp[0x11] = 0x54;

		temp[0x12] = 0xC1;//最后访问日期
		temp[0x13] = 0x54;

		temp[0x16] = 0x06;//修改时间
		temp[0x17] = 0x7A;

		temp[0x18] = 0xC1;//修改日期
		temp[0x19] = 0x54;

		AddContentToCluster(node->FirstCluster, buffer, total_content_num * 32ull);

		delete node;
		delete[] buffer;
		delete[] section1;
		delete[] section2;
		return ERR_None;
	}

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
	FAT32FileNode* node = (FAT32FileNode*)FindFileByPath(path);
	if (node == nullptr)
	{
		return ERR_FilePathNotExist;
	}
	unsigned char buffer[1];
	buffer[0] = 0xE5;
	WriteRawData(node->ContentLba, node->ContentOffset, 1,buffer);
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
	FAT32FileNode* node = new FAT32FileNode(this,0,0,0);
	node->IsDir = attr & (1 << 4);
	if (node->IsDir)
		node->Attributes |= FileNode::A_Dir;
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
			if (file_name[i] <= 'Z' && file_name[i] >= 'A')
			{
				file_name[i] += 32;
			}
		}
	}
	if (buffer[0x0C] == 0x10 || buffer[0x0C] == 0x18)
	{
		for (int i = file_name_length + 1; i <total_length; i++)
		{
			if (file_name[i] <= 'Z' && file_name[i] >= 'A')
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
	delete [] file_name; //移植的时候记得加上
	return node;

}
ErrorType FAT32::LoadLongFileNameFromBuffer(unsigned char* buffer, Uint32*  name)
{
	name[0]  = (buffer[0x02] << 8) | (buffer[0x01]);
	name[1]  = (buffer[0x04] << 8) | (buffer[0x03]);
	name[2]  = (buffer[0x06] << 8) | (buffer[0x05]);
	name[3]  = (buffer[0x08] << 8) | (buffer[0x07]);
	name[4]  = (buffer[0x0A] << 8) | (buffer[0x09]);
		    					
	name[5]  = (buffer[0x0F] << 8) | (buffer[0x0E]);
	name[6]  = (buffer[0x11] << 8) | (buffer[0x10]);
	name[7]  = (buffer[0x13] << 8) | (buffer[0x12]);
	name[8]  = (buffer[0x15] << 8) | (buffer[0x14]);
	name[9]  = (buffer[0x17] << 8) | (buffer[0x16]);
	name[10] = (buffer[0x19] << 8) | (buffer[0x18]);
								
	name[11] = (buffer[0x1D] << 8) | (buffer[0x1C]);
	name[12] = (buffer[0x1F] << 8) | (buffer[0x1E] );
	return ERR_None;
}
char* FAT32::MergeLongNameAndToUtf8(Uint32* unicode[], Uint32 cnt)
{

	char* result = new char[4 * 13 * cnt];
	Uint64 last_length = 0;
	for (int i = cnt - 1; i >= 0; i--)
	{
		last_length = POS::UnicodeToUtf8(result+last_length, unicode[i], 13);
	}
	return result;
}
Uint64 FAT32::GetLbaFromCluster(Uint64 cluster)
{
	return RootLba + (cluster - 2)*Dbr.BPBSectorPerClus;

}
Uint64 FAT32::GetSectorOffsetFromlba(Uint64 lba)//当前lba是所属簇的第几个扇区
{
	lba -= RootLba;
	if (lba < 0)
	{
		Panic("try to get sector offset from a lba out of data range!" );
		return -1;
	}
	return lba / Dbr.BPBSectorPerClus;
}

//FileNode* FAT32::GetFileNodesFromCluster(Uint64 cluster) // cluster对应的是目录项
//{
//	FAT32FileNode* head = nullptr,* cur = nullptr;
//	Uint64 lba = GetLbaFromCluster(cluster);
//	for (Uint32 i = 0; i < Dbr.BPBSectionPerClus; i++)
//	{
//		unsigned char buffer[SECTORSIZE];
//		ReadRawData(lba+i, 0, 512, buffer);
//		for (Uint32 j = 0; j < SECTORSIZE / 32; j++)
//		{
//			unsigned char temp[32];
//			POS::MemcpyT(temp, buffer + j * 32, 32);
//			Uint16 attr = temp[11];
//			if (attr == 0x0F)//长目录项
//			{
//
//			}
//			else //短目录项
//			{
//				FAT32FileNode* p = (FAT32FileNode*)LoadShortFileInfoFromBuffer(temp);
//				if (p == nullptr)
//				{
//					continue;
//				}
//				
//				if (head == nullptr)
//				{
//					head = p;
//					cur = head;
//					cur->nxt = nullptr;
//				}
//				else
//				{
//					cur->nxt = (FAT32FileNode*)LoadShortFileInfoFromBuffer(temp);
//					cur = cur->nxt;
//					cur->nxt = nullptr;
//				}
//			}
//		}
//	}
//	Uint64 nxt = GetFATContentFromCluster(cluster);
//	if (nxt != CLUSTEREND)
//	{
//
//	}
//	return head;
//	
//}
Uint32 FAT32::GetFATContentFromCluster(Uint32 cluster)
{
	Uint64 lba = FAT1Lba + (Uint64)cluster * 4 / SECTORSIZE;//对应扇区lba
	Uint64 offset = ((Uint64)cluster * 4) % SECTORSIZE;
	unsigned char buffer[4];
	ReadRawData(lba, offset, 4, buffer);
	if (buffer[0]==0xf8) 
		buffer[0]=0xff;
	return ((Uint64)buffer[3] << 24) | ((Uint64)buffer[2] << 16) | ((Uint64)buffer[1] <<8) | buffer[0];
}
ErrorType FAT32::SetFATContentFromCluster(Uint32 cluster, Uint32 content)//设置cluster对应的FAT表中内容为content(自动将content转换为大端)
{
	unsigned char buffer[4];
	buffer[0] = content & 0x000000FF;
	buffer[1] = (content & 0x0000FF00) >> 8;
	buffer[2] = (content & 0x00FF0000) >> 16;
	buffer[3] = (content & 0xFF000000) >> 24;
	Uint64 lba = FAT1Lba + (Uint64)cluster * 4ull / SECTORSIZE;//对应扇区lba
	Uint64 offset = ((Uint64)cluster * 4ull) % SECTORSIZE;
	return WriteRawData(lba, offset, 4, buffer);
}
FileNode* FAT32::FindFileByNameFromCluster(Uint32 cluster, const char* name)
{
	FAT32FileNode* result;
	
	Uint64 lba = GetLbaFromCluster(cluster);

	int long_name_cnt = 0;//长文件名计数
	Uint32* long_name[100];//存储长文件名

	while (cluster != CLUSTEREND)
	{
		for (Uint32 i = 0; i < Dbr.BPBSectorPerClus; i++)
		{
			unsigned char buffer[SECTORSIZE];
			ReadRawData(lba + i, 0, 512, buffer);
			for (Uint32 j = 0; j < SECTORSIZE / 32; j++)
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
					Uint32* temp_long_name = new Uint32[13];
					LoadLongFileNameFromBuffer(temp, temp_long_name);
					long_name[long_name_cnt] = temp_long_name;
					long_name_cnt++;
				}
				else //短目录项
				{
					result = (FAT32FileNode*)LoadShortFileInfoFromBuffer(temp);
					
					if (result == nullptr)
					{
						for (int i = 0; i < long_name_cnt; i++)
						{
							delete[] long_name[i];
						}
						long_name_cnt = 0;
						continue;
					}
					result->ContentLba = lba + i;
					result->ContentOffset = j * 32;
					if (long_name_cnt) //如果是长目录对应的短目录
					{
						char* long_full_name = MergeLongNameAndToUtf8(long_name, long_name_cnt);
						result->SetFileName(long_full_name,false);
						delete [] long_full_name;
						for (int i = 0; i < long_name_cnt; i++)
						{
							delete[] long_name[i];
						}
						long_name_cnt = 0;
					}

					if (POS::strComp(name, result->Name) == 0)
					{
						return result;
					}
					else
					{
						delete result;
					}
					
				}
				
			}
		}
		cluster = GetFATContentFromCluster(cluster);
	
	}
	
	return nullptr;
}
FileNode* FAT32::FindFileByPath(const char* path)
{
	if (POS::strLen(path) == 0)
	{
		return nullptr;
	}
	if (path[0] != '/')
	{
		kout << "now this system only support absolute path,which means begin with /";
		return nullptr;
	}
	if (POS::strLen(path) == 1)
	{
		FAT32FileNode* node = new FAT32FileNode(this, 2,GetLbaFromCluster(2),0);
		node->IsDir = true;
		node->Attributes |= FileNode::A_Dir;
		return node;
	}


	FAT32FileNode* node = new FAT32FileNode(this, 2,GetLbaFromCluster(2),0);
	Uint64 index, last_index = 1;
 	for (index= 1; index < POS::strLen(path); index++)
	{
		if (path[index] == '/')
		{
			if (index == last_index)
			{
				delete node;
				return nullptr;
			}
			char* temp = new char[(Uint32)(index - last_index + 1ull)];
			MemcpyT(temp, path + last_index, index - last_index);
			temp[index - last_index] = '\0';
			FAT32FileNode* last_node = node;
			node = (FAT32FileNode*)FindFileByNameFromCluster(node->FirstCluster,temp);

			delete[] temp;
			delete last_node;
			if (node == nullptr)
			{
				return nullptr;
			}
			
			if (index != POS::strLen(path) && node->IsDir == false)
			{
				delete node;
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
		node = (FAT32FileNode*)FindFileByNameFromCluster(node->FirstCluster, temp);
		delete[] temp;
		delete last_node;
	}
	return node;
}
bool FAT32::IsExist(const char* path)
{
	return FindFileByPath(path) != nullptr;
}

Uint32 FAT32::GetFreeClusterAndPlusOne()//返回一个空闲簇的簇号
{
	unsigned char buffer[8];
	ReadRawData(DBRLba + 1ull,0x1E8,8,buffer);
	Uint32 free_cluster = (buffer[7] << 24) | (buffer[6] << 16) | (buffer[5] << 8) | buffer[4];
	Uint32 free_cluster_num = (buffer[3] << 24) | (buffer[2] << 16) | (buffer[1] << 8) | buffer[0];

	Uint32 new_free_cluster = free_cluster + 1;
	free_cluster_num--;

	buffer[0] = free_cluster_num & 0x000000FF;
	buffer[1] = (free_cluster_num & 0x0000FF00) >> 8;
	buffer[2] = (free_cluster_num & 0x00FF0000) >> 16;
	buffer[3] = (free_cluster_num & 0xFF000000) >> 24;

	buffer[4] = new_free_cluster & 0x000000FF;
	buffer[5] = (new_free_cluster & 0x0000FF00) >> 8;
	buffer[6] = (new_free_cluster & 0x00FF0000) >> 16;
	buffer[7] = (new_free_cluster & 0xFF000000) >> 24;

	WriteRawData(DBRLba + 1,0x1E8, 8, buffer);

	SetFATContentFromCluster(free_cluster, CLUSTEREND);
	return free_cluster;
}
PAL_DS::Doublet<Uint64, Uint64> FAT32::GetContentLbaAndOffsetFromPath()//得到文件所在目录项的位置，例如要删除文件就要把文件对应目录项设置为E5
{
	return PAL_DS::Doublet<Uint64, Uint64>(0,0);
}
PAL_DS::Triplet<Uint32, Uint64,Uint64> FAT32::GetFreeClusterAndLbaAndOffsetFromCluster(Uint32 cluster)//得到目录cluster中下一个空白的位置用于放置目录项
{
	while (true)
	{
		Uint64 lba = GetLbaFromCluster(cluster);
		for (Uint32 i = 0; i < Dbr.BPBSectorPerClus; i++)
		{
			unsigned char buffer[SECTORSIZE];
			ReadRawData(lba + i, 0, 512, buffer);
			for (Uint32 j = 0; j < SECTORSIZE / 32; j++)
			{
				if(*(buffer + j * 32) == 0x00 )
				{
					return PAL_DS::Triplet<Uint32, Uint64, Uint64>(cluster,lba + i, (Uint64)j * 32);
				}
			}
		}
		Uint32 last_cluster = cluster;
		cluster = GetFATContentFromCluster(cluster);
		if (cluster == CLUSTEREND)
		{
			cluster = GetFreeClusterAndPlusOne();
			SetFATContentFromCluster(last_cluster, cluster);
			SetFATContentFromCluster(cluster, CLUSTEREND);
			return PAL_DS::Triplet<Uint32, Uint64, Uint64>(cluster,GetLbaFromCluster(cluster), 0);;
		}
	}

}

ErrorType FAT32::AddContentToCluster(Uint32 _cluster, unsigned char* buffer, Uint64 size)
{
	Triplet <Uint32, Uint64,Uint64> cluster_lba_offset = GetFreeClusterAndLbaAndOffsetFromCluster(_cluster);
	Uint32 cluster = cluster_lba_offset.a;
	Uint64 lba = cluster_lba_offset.b, offset = cluster_lba_offset.c;
	if (size % 32 != 0)//目录项必须是32的倍数
	{
		return ERR_InvalidParameter;
	}
	Uint64 write_bytes = 0;
	Uint64 sector_offset = GetSectorOffsetFromlba(lba);
	for (int i = 0; i < size / 32; i++)
	{
		WriteRawData(lba, offset, 32, buffer+write_bytes);
		offset += 32;
		write_bytes += 32;
		if (offset % SECTORSIZE == 0)
		{
			lba++;
			offset = 0;
			++sector_offset;
		}
		if(sector_offset == Dbr.BPBSectorPerClus)
		{
			Uint32 last_cluster = cluster;
			cluster = GetFATContentFromCluster(cluster);
			if (cluster == CLUSTEREND)
			{
				cluster = GetFreeClusterAndPlusOne();
				SetFATContentFromCluster(last_cluster, cluster);
				SetFATContentFromCluster(cluster, CLUSTEREND);
				lba = GetLbaFromCluster(cluster);
			}
		}
	}
	return ERR_None;

}

unsigned char FAT32::CheckSum(unsigned char* data)
{
	short name_len;
	unsigned char sum;  //必须为无符号型.

	sum = 0;
	for (name_len = 11; name_len != 0; name_len--) {
		sum = ((sum & 1) ? 0x80 : 0) + (sum >> 1) + *data++;
	}
	return (sum);
}

