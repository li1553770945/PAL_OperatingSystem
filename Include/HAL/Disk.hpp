#ifndef POS_DISK_HPP
#define POS_DISK_HPP

#include "../Types.hpp"

constexpr int SectorSize=512;
struct Sector
{
	Uint8 data[SectorSize];
	
	inline Uint8& operator [] (unsigned pos)
	{return data[pos];}
	
	inline const Uint8& operator [] (int i) const
	{return data[i];}
};

ErrorType DiskInit();
ErrorType DiskReadSector(unsigned long long LBA,Sector *sec,int cnt=1);
ErrorType DiskWriteSector(unsigned long long LBA,const Sector *sec,int cnt=1);
ErrorType DiskInterruptSolve();

#endif
