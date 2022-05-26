#ifndef POS_DISK_HPP
#define POS_DISK_HPP

struct Sector
{
	Uint8 data[SectorSize];
	
	inline Uint8& operator [] (unsigned pos)
	{return data[pos];}
};

ErrorType Wrench_ReadSector(unsigned long long LBA,Sector sec[],int cnt=1);
ErrorType Wrench_WriteSector(unsigned long long LBA,Sector sec[],int cnt=1);

#endif
