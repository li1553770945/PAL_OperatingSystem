#ifndef CONST_HPP
#define CONST_HPP

#include <Types.hpp>

constexpr Uint32 PageSizeBit=12,
				 PageSize=1<<PageSizeBit;

constexpr Uint64 PageSizeN[3]{PageSize,PageSize*512,PageSize*512*512};
					   
constexpr Uint64 PhysicalVirtualMemoryOffset=0xFFFFFFFF40000000ull,
				 PhysicalKernelStartAddr=0x80020000ull;

constexpr Uint64 PhysicalMemorySize()
{return 0x5F0000;}

constexpr Uint64 PhysicalMemoryPhysicalStart()
{return 0x80000000;}

constexpr Uint64 PhymemVirmemOffset()
{return PhysicalVirtualMemoryOffset;}

constexpr Uint64 PhysicalMemoryVirtualEnd()
{return PhymemVirmemOffset()+PhysicalMemoryPhysicalStart()+PhysicalMemorySize();}

extern "C"
{
	extern char kernelstart[];
	extern char textstart[];
	extern char textend[];
	extern char rodatastart[];
	extern char rodataend[];
	extern char datastart[];
	extern char dataend[];
	extern char bssstart[];
	extern char bssend[];
	extern char kernelend[];
	extern char freememstart[];
	extern char bootstack[];
	extern char bootstacktop[];
	
	inline Uint64 FreeMemBase()
	{return (Uint64)freememstart;}
};



#endif