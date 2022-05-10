#ifndef POS_PHYSICALMEMORY_HPP
#define POS_PHYSICALMEMORY_HPP

#include "../Types.h"

const unsigned int PageSizeBit=12;
const unsigned int PageSize=1<<12;

inline Uint64 PhysicalMemorySize()
{return 0x7E00000;}

inline Uint64 PhymemVirmemOffset()
{return 0xFFFFFFFFc0000000ull;}

extern "C" {
extern char freememstart[];
inline Uint64 FreeMemBase()
{
	
	return (Uint64)freememstart;
}
}

struct Page
{
	Uint64 flags, //
		   num; //后面有几个页属于他管理
	Page *pre,
		 *nxt;
	inline Uint64 Index() const
	{return ((Uint64)this-(Uint64)FreeMemBase())/sizeof(Page);}
	
	inline void* Addr() const
	{return (void *)(FreeMemBase()+Index()*PageSize);}
};

class PhysicalMemoryManager
{	
    Page head;  //相当于链表头节点，不存放数据，head->nxt为链表的第一个节点
	Page *page_end_addr;
    Uint64 page_count;
	void MergePage(Page * p);
	public:
		Page * GetPageFromAddr(void * addr);
		const char* Name() const;
        int Init();
		Page* AllocPage(unsigned long long count);
		void Free(Page *p);
		
		~PhysicalMemoryManager();
		PhysicalMemoryManager();
};
extern PhysicalMemoryManager POS_PMM;

void *kmalloc(Uint64);
void kfree(void *);
#endif