#include <Memory/PhysicalMemory.hpp>
#include <Library/Kout.hpp>
using namespace POS;
Page* PhysicalMemoryManager::AllocPage(unsigned long long count)
{
    Page *p=head.nxt;
    while(p)
    {
        if(count<p->num)
        {
            Page * pre = p->pre;
            int num = p->num;
            p->num=count;
            p->pre->nxt = p + count;
            p->pre->nxt->pre = pre;            
            p->pre->nxt->num = num - count;
            return p;
        }
        else if(p->num==count)
        {
            Page * pre = p->pre;
            p->pre->nxt = p->nxt;
            if(p->nxt)
            {
                p->nxt->pre = p->pre;
            }
            return p;
        }
        p = p->nxt;

    }
    return nullptr;
}
void PhysicalMemoryManager::Free(Page *p)
{
    
}
int PhysicalMemoryManager::Init()
{
    Page * page = (Page*)FreeMemBase();
    Uint64 page_count = (PhysicalMemorySize() + PhymemVirmemOffset() -  FreeMemBase()) / PageSize;
    using namespace POS;
    kout << "page_cnt:"<<page_count<<endl;
    for(int i=0;i<page_count;i++)
    {
        page[i].flags = 0;
    }
    Uint64 page_need_size = page_count * sizeof(Page); // 存储所有的page需要的空间
    Uint64 page_need_page_num = page_need_size / PageSize + 1; //存储所有的page需要的page的数量
    kout << "page_need_page_num:"<<page_need_page_num<<endl;
    page[0].num = page_need_page_num;
    page[0].flags = 1;
    

    page[page_need_page_num].num = page_count - page_need_page_num;
    page[page_need_page_num].nxt = nullptr;
    head.nxt = page + page_need_page_num;
    page[page_need_page_num].pre = &head;

    return 0;
}
PhysicalMemoryManager::~PhysicalMemoryManager()
{

}
PhysicalMemoryManager::PhysicalMemoryManager()
{
    
}
const char* PhysicalMemoryManager::Name()  const
{
    return "default";
}


PhysicalMemoryManager POS_PMM;
