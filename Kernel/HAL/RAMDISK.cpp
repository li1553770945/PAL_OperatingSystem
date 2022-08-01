#include <HAL/Disk.hpp>
#include <Memory/PhysicalMemory.hpp>
#include <Library/Kout.hpp>
using namespace POS;

constexpr Uint64 RAMDISK_BaseP=0x84200000,
				 RAMDISK_BaseK=RAMDISK_BaseP+PhysicalVirtualMemoryOffset,
				 RAMDISK_SectorCount=4*1024*1024/SectorSize;

ErrorType DiskInit()
{
	kout[Info]<<"Disk init OK"<<endl;
	return ERR_None;
}

ErrorType DiskReadSector(unsigned long long LBA,Sector *sec,int cnt)
{
	if (LBA>=RAMDISK_SectorCount)
		return ERR_DiskRwOutOfRange;
	MemcpyT(sec,(Sector*)RAMDISK_BaseK+LBA,cnt);
	return ERR_None;
}

ErrorType DiskWriteSector(unsigned long long LBA,const Sector *sec,int cnt)
{
	if (LBA>=RAMDISK_SectorCount)
		return ERR_DiskRwOutOfRange;
	MemcpyT((Sector*)RAMDISK_BaseK+LBA,sec,cnt);
	return ERR_None;
}

ErrorType DiskInterruptSolve()
{return ERR_Unknown;}
