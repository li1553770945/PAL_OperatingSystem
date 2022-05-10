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
            p->flags = 1;
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
            p->flags = 1;
            return p;
        }
        p = p->nxt;

    }
    return nullptr;
}
void  PhysicalMemoryManager::MergePage(Page * p)//检查p能否和后面一个位置合并
{
    if(p+p->num<page_end_addr&&(p+p->num)->flags==0)//如果后面一个没有使用
    {
        Page * nxt = p+p->num;
        p->num+=nxt->num;
        p->nxt = nxt->nxt;
        if(nxt->nxt)
        {
            nxt->nxt->pre = p;
        }

    }
}
void PhysicalMemoryManager::Free(Page *p)
{
    p->flags = 0;
    Page * pre_p = head.nxt;
    if(pre_p==nullptr||pre_p > p) //如果已经没有空闲空间，或者第一个空闲空间也在p的后面，则p直接插入到head后面
    {
        
        Page * nxt_p = pre_p;
        head.nxt = p;
        p->pre = &head;
        p->nxt = nxt_p;
        if(nxt_p)
        {
            nxt_p->pre = p;
        }
        MergePage(p);
        return;
    }

    while(pre_p->nxt&&pre_p->nxt < p)//这样pre_p就是最后一个小于p的位置
    {
        pre_p = pre_p->nxt;
    }

    Page* nxt_p = pre_p->nxt;//p位于pre_p和next_p之间
    pre_p ->nxt = p;
    p->pre = pre_p;

    p->nxt = nxt_p;
    if(nxt_p)
    {
        nxt_p->pre = p;
    }
    MergePage(p);

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
    page_end_addr = page + page_count;
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

Page * PhysicalMemoryManager::GetPageFromAddr(void * addr)
{
    return  (Page*)(void*)(FreeMemBase() + ((Uint64)addr -FreeMemBase()) / PageSize * sizeof(Page));
}

PhysicalMemoryManager POS_PMM;

void * kmalloc(Uint64 size)
{

    Uint64 page_num = size / PageSize;
    if(size % PageSize)
    {
        page_num++;
    }
    Page * p = POS_PMM.AllocPage(page_num);
    return p->Addr();
}

void  kfree(void *p)
{
    POS_PMM.Free(POS_PMM.GetPageFromAddr(p));
}


