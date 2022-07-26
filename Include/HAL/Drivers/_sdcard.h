#ifndef __SDCARD_H
#define __SDCARD_H

#include "../Disk.hpp"

extern "C"
{
	#include <HAL/Drivers/_plic.h>
	#include <HAL/Drivers/_fpioa.h>
	#include <HAL/Drivers/_dmac.h>
};
#include <HAL/Drivers/_sdcard.h>

void sdcard_init(void);
void sdcard_read_sector(Sector *sec,int sectorno);
void sdcard_write_sector(const Sector *sec,int sectorno);
void test_sdcard(void);

#endif 
