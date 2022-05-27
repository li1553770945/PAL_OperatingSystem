#ifndef __SDCARD_H
#define __SDCARD_H

#include "../../Types.hpp"

constexpr int SectorSize=512;
struct Sector
{
	Uint8 data[SectorSize];
	
	inline Uint8& operator [] (int i)
	{return data[i];}
	
	inline const Uint8& operator [] (int i) const
	{return data[i];}
};

void sdcard_init(void);

void sdcard_read_sector(Sector *sec,int sectorno);

void sdcard_write_sector(const Sector *sec,int sectorno);

void test_sdcard(void);

#endif 
