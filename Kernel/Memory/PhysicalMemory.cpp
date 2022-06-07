#include <Memory/PhysicalMemory.hpp>
#include <Library/Kout.hpp>
#include <Library/Math.hpp>
#include <Memory/Buddy.hpp>
#include <Memory/Slab.hpp>
#include <Memory/Page.hpp>

using namespace POS;

void* PhysicalMemoryManager::Alloc(Uint64 size)//要分配的字节数
{
    int need_page_num = size/PageSize;
    if(size%PageSize)
        need_page_num++;
    return (void*)zone.AllocPage(need_page_num)->addr;
}

void PhysicalMemoryManager::Free(void *p)
{
    Page * page_addr = zone.GetPageFromAddr(p);
    zone.FreePage(page_addr);

}

ErrorType PhysicalMemoryManager::Init()
{
    kout[Info]<<"Initing PhysicalMemoryManager..."<<endl;
	MemsetT<Uint64>((Uint64*)FreeMemBase(),0,(PhysicalMemoryVirtualEnd()-FreeMemBase())/sizeof(Uint64));//Init free memory to 0 in k210
	int code = zone.Init();
	if(code)
    {
        return code;
    }
    kout[Info]<<"Init PhysicalMemoryManager Success"<<endl;
    return 0;
}

const char* PhysicalMemoryManager::Name()  const
{
    return "POS_DefaultPMM";
}

Page* PhysicalMemoryManager::AllocPage(Uint64 count)//Allocate a continous range of pages with start addr returned.
{
    return (Page*)zone.AllocPage(count);
}
		
void PhysicalMemoryManager::FreePage(Page *p)
{
    zone.FreePage(p);
}
		
Page* PhysicalMemoryManager::GetPageFromAddr(void *addr)
{
    return zone.GetPageFromAddr(addr);
}
        

PhysicalMemoryManager POS_PMM;
