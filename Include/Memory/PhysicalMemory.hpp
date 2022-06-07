#ifndef POS_PHYSICALMEMORY_HPP
#define POS_PHYSICALMEMORY_HPP

#include "../Types.hpp"
#include "../Error.hpp"
#include <Memory/Page.hpp>
#include <Memory/Buddy.hpp>
#include <Memory/Slab.hpp>

class PhysicalMemoryManager
{
	protected:
    	Zone zone;

	public:
		const char* Name()  const;
		void* Alloc(Uint64 size);
		void Free(void *addr);
		
		Page* GetPageFromAddr(void *addr);
		
		Page* AllocPage(Uint64 count);//Allocate a continous range of pages with start addr returned.
		void FreePage(Page *p);
	
		
		ErrorType Init();
};


extern PhysicalMemoryManager POS_PMM;

inline void* Kmalloc(Uint64 size)
{return POS_PMM.Alloc(size);}

inline void Kfree(void *addr)
{
	if (addr)
		POS_PMM.Free(addr);
}

template <typename T> inline T* KmallocT()
{return (T*)POS_PMM.Alloc(sizeof(T));}

#endif
