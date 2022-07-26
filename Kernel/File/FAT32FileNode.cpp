#pragma once
#include <File/FAT32.hpp>
#include <Types.hpp>
#include <Library/Kout.hpp>
using namespace POS;
FAT32FileNode::FAT32FileNode(FAT32* _vfs, Uint32 _cluster, Uint64 _ContentLba, Uint64 _ContentOffset) :FileNode(_vfs, 0, 0)
{
	IsDir = false;
	nxt = nullptr;
	Vfs = _vfs;
	FirstCluster = _cluster;
	ReadSize = 0;
}

Sint64 FAT32FileNode::Read(void* dst, Uint64 pos, Uint64 size)
{
	CALLINGSTACK
	if (IsDir)
	{
		return -ERR_PathIsNotFile;
	}
	FAT32* vfs = (FAT32*)Vfs;
	Sint64 total_has_read_size = 0;//可能要跨扇区、簇读取，这是本次（该函数执行完一次）总数据量
	Uint64 bytes_per_cluster = SECTORSIZE * vfs->Dbr.BPBSectorPerClus;
	if (pos >= FileSize)
	{
		return -ERR_FileOperationOutofRange;
	}
	if(pos + size > FileSize)
	{
		size = FileSize - pos;
	}
	if (size == 0)
	{
		return 0;
	}
	PAL_DS::Doublet <Uint32, Uint64> cluster_and_lba = GetCLusterAndLbaFromOffset(pos);
	Uint32 cluster = cluster_and_lba.a;
	Uint64 lba = cluster_and_lba.b;

	Uint64 sector_offset = pos % SECTORSIZE;
	Uint64 cluster_offset = pos % bytes_per_cluster;//当前簇读的位置，用于判断是否该切换下一个lba和簇


	while (size)
	{
		if (IsClusterEnd(cluster))
		{
			kout[Error] << "try to read FAT32 from cluster end" << endl;
			return -ERR_InvalidClusterNumInFAT32;
		}
		Uint64 sector_need_read_size;//当前扇区需要读取的字节
		if (sector_offset + size <= SECTORSIZE)//这个扇区可以直接满足
		{
			sector_need_read_size = size;
			size = 0;
		}
		else//当前扇区不能满足，把这个扇区读完
		{
			sector_need_read_size = SECTORSIZE - sector_offset;
			size -= sector_need_read_size;
		}
		
//		kout[Debug]<<"LBA "<<lba<<" "<<sector_need_read_size<<" "<<size<<" "<<cluster<<endl;
		vfs->ReadRawData(lba, sector_offset, sector_need_read_size, (unsigned char*)dst + total_has_read_size);
		total_has_read_size += sector_need_read_size;
		cluster_offset += sector_need_read_size;
		sector_offset += sector_need_read_size;
		if (sector_offset == SECTORSIZE)//这个扇区已经读完
		{
			lba++;
			sector_offset = 0;
		}
		if (cluster_offset == bytes_per_cluster) //这个簇已经读完
		{
			cluster = vfs->GetFATContentFromCluster(cluster);
			lba = vfs->GetLbaFromCluster(cluster);//新簇LBA
			cluster_offset = 0;
		}
	}
	
//	kout[Debug]<<"thrs "<<total_has_read_size<<endl;
	return total_has_read_size;
}
PAL_DS::Doublet <Uint32, Uint64> FAT32FileNode::GetCLusterAndLbaFromOffset(Uint64 offset)
{
	FAT32* vfs = (FAT32*)Vfs;
	Uint64 lba;
	Uint64 size_per_cluster = SECTORSIZE * vfs->Dbr.BPBSectorPerClus;
	Uint32 cluster = FirstCluster;
	while (offset >= size_per_cluster)
	{
		offset -= size_per_cluster;
		cluster = vfs->GetFATContentFromCluster(cluster);
	}
	lba = vfs->GetLbaFromCluster(cluster) + offset / SECTORSIZE;
	return PAL_DS::Doublet <Uint32, Uint64>(cluster, lba);
}
ErrorType FAT32FileNode::SetSize(Uint32 size)//设置文件大小，只能缩小，不能放大
{
	if (size > FileSize)
	{
		return ERR_FileOperationOutofRange;
	}
	else if(size == FileSize)
	{
		return ERR_None;
	}
	FileSize = size;
	unsigned char  size_buffer[4];
	size_buffer[0] = (FileSize & 0x000000FF);
	size_buffer[1] = (FileSize & 0x0000FF00) >> 8;
	size_buffer[2] = (FileSize & 0x00FF0000) >> 16;
	size_buffer[3] = (FileSize & 0xFF000000) >> 24;
	FAT32* vfs = (FAT32*)Vfs;
	vfs->WriteRawData(ContentLba, ContentOffset + 28, 4, size_buffer);
	return ERR_None;
}

Sint64 FAT32FileNode::Write(void* src, Uint64 pos, Uint64 size)
{
	Uint64 size_bak = size;
	if (IsDir)
	{
		return -ERR_PathIsNotFile;
	}
	if (pos > FileSize)
	{
		return -ERR_FileOperationOutofRange;
	}
	FAT32* vfs = (FAT32*)Vfs;
	Uint64 total_has_write_size = 0;//可能要跨扇区、簇读取，这是本次（该函数执行完一次）总数据量
	Uint64 bytes_per_cluster = SECTORSIZE * vfs->Dbr.BPBSectorPerClus;

	if (size == 0)
	{
		return 0;
	}
	PAL_DS::Doublet <Uint32, Uint64> cluster_and_lba = GetCLusterAndLbaFromOffset(pos);
	Uint32 cluster = cluster_and_lba.a;
	Uint64 lba = cluster_and_lba.b;

	Uint64 sector_offset = pos % SECTORSIZE;
	Uint64 cluster_offset = pos % bytes_per_cluster;//当前簇读的位置，用于判断是否该切换下一个lba和簇

	while (size)
	{
	
		Uint64 sector_need_write_size;//当前扇区需要写入的字节
		if (sector_offset + size <= SECTORSIZE)//这个扇区可以直接满足
		{
			sector_need_write_size = size;
			size = 0;
		}
		else//当前扇区不能满足，把这个扇区读完
		{
			sector_need_write_size = SECTORSIZE - sector_offset;
			size -= sector_need_write_size;
		}

		vfs->WriteRawData(lba, sector_offset, sector_need_write_size, (unsigned char*)src + total_has_write_size);
		total_has_write_size += sector_need_write_size;
		cluster_offset += sector_need_write_size;
		sector_offset += sector_need_write_size;
		if (sector_offset == SECTORSIZE)//这个扇区已经读完
		{
			lba++;
			sector_offset = 0;
		}
		if (cluster_offset == bytes_per_cluster) //这个簇已经读完
		{
			Uint32 last_cluster = cluster;
			cluster = vfs->GetFATContentFromCluster(cluster);
			if (IsClusterEnd(cluster))
			{
				cluster = vfs->GetFreeClusterAndPlusOne();
				vfs->SetFATContentFromCluster(last_cluster, cluster);
			}
			lba = vfs->GetLbaFromCluster(cluster);//新簇LBA
			cluster_offset = 0;
		}
	}
	if (pos + size_bak > FileSize)
	{
		FileSize += pos + size_bak - FileSize;
		unsigned char  size_buffer[4];
		size_buffer[0] = (FileSize & 0x000000FF);
		size_buffer[1] = (FileSize & 0x0000FF00) >> 8;
		size_buffer[2] = (FileSize & 0x00FF0000) >> 16;
		size_buffer[3] = (FileSize & 0xFF000000) >> 24;
		vfs->WriteRawData(ContentLba, ContentOffset + 28, 4, size_buffer);
	}
	
	return total_has_write_size;
}

FAT32FileNode::~FAT32FileNode()
{

}

