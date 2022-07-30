#include <Memory/PhysicalMemory.hpp>
#include <Library/Kout.hpp>
#include <Library/Math.hpp>
using namespace POS;

Page * Page::DismantleNext()
{
    Page * next = this->next;
    if(next==nullptr)
    {
        //panic("DismantleNext Failed");
        return nullptr;
    }
    this->next = next->next;
    if(next->next)
        next->next->pre = this;

    return next;
}
Page * Page::Dismantle()
{
    this->pre->next = this->next;
    if(this->next)
    {
        this->next->pre = this->pre;
    }
    return this;
}
void Page::AddNext(Page * next)
{
    next->next = this->next;
    this->next = next;
    next->pre = this;
    if(next->next)
        next->next->pre = next;
}

void* PhysicalMemoryManager::Alloc(Uint64 size,bool success)//要分配的字节数
{
    int need_page_num = size/PageSize;
    if(size%PageSize)
        need_page_num++;
    Page *page=zone.AllocPage(need_page_num);
    if (page==nullptr&&success)
    	kout[Fault]<<"PhysicalMemoryManager::Alloc failed to allocate! Size "<<size<<" FreePages "<<GetFreePageNum()<<endl;
    return (void*)page->addr;
}

void PhysicalMemoryManager::Free(void *p)
{
    Page * page_addr = zone.GetPageFromAddr(p);
    if (page_addr==nullptr)
    	kout[Error]<<"Failed to free "<<p<<endl;
    else zone.FreePage(page_addr);
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

PhysicalMemoryManager POS_PMM;

void Zone::BuildTree(int pos,int left,int right,int x,int y,Uint64 addr)
{
    page[pos].addr = addr;
    if(left==right)
    {
        page[pos].flags = 0;
        return;
    }
    int mid = (left+right)/2;
    if(x<=left&&y>=right)
    {
        page[pos].flags = 0;
    }
    if(x<=mid)
    {
        BuildTree(pos*2,left,mid,x,y,addr);
    }
    if(y>=mid+1)
    {
        BuildTree(pos*2+1,mid+1,right,x,y,addr + kpow2(page[pos].order-1)*PageSize);
    }
}

void Zone::InitTree(int index,int order,int left,int right)
{
//	kout[Debug]<<"Zone::InitTree "<<index<<" "<<order<<" "<<left<<" "<<right<<endl;
    page[index].index = index;
    page[index].flags = -1;
    page[index].addr = 0;
    page[index].pre = nullptr;
    page[index].next = nullptr;
    page[index].order = order;

    if(left == right)
    {
        return;
    }
    int mid = (left+right)/2;
    InitTree(index * 2,order-1,left,mid);
    InitTree(index * 2 + 1,order-1,mid + 1,right);
}
void Zone::InitArea(int index,int left,int right)
{
    if(page[index].flags == 0)
    {
        free_area[page[index].order].head.next = &page[index];
        page[index].pre = &free_area[page[index].order].head;
        return;
    }
    if(left==right)
    {
        return;
    }
    int mid = (left+right)/2;
    InitArea(index*2,left,mid);
    InitArea(index*2+1,mid+1,right);

}
int Zone::Init()
{
    physical_memory_size = PhysicalMemorySize();
    page_size = PageSize;
    end_addr = PhysicalMemoryVirtualEnd();
    page_base = (Uint64)freememstart;
    page = (Page*)((void*)page_base);
    page->next = nullptr;

    int height = klog2((end_addr - page_base)/page_size) + 2;

    max_order = height - 1;
    page_num = kpow2(height) - 1;
    int right = kpow2(height -1);
    int left = 1;
    InitTree(1,max_order,left,right);
    page_need_memory = page_num * sizeof(Page);
    free_memory_start_addr =  page_base + page_need_memory+4095>>PageSizeBit<<PageSizeBit;
    free_memory_size = end_addr - free_memory_start_addr;
    valid_page_num =  free_memory_size / page_size;
    free_page_num = valid_page_num;
    BuildTree(1,left,right,1,valid_page_num,free_memory_start_addr);
    for(int i=0;i<=max_order;i++)
    {
        free_area[i].head.next = nullptr;
    }
    InitArea(1,left,valid_page_num);
    return 0;
    
    
}
Page * Zone::AllocPage(Uint64 need_page_num)
{
	ISASBC;//??
	
    int order = klog2(need_page_num);
    if((int)kpow2(order) != need_page_num)
    {
        order++;
    }

    int cur_order = order;
    
    do{
        if(free_area[cur_order].head.next != nullptr)
        {
            break;
        }
    }while(++cur_order<=max_order);
    if(cur_order > max_order)
    {
    	kout[Error]<<"PhysicalMemoryManager::Failed to allocate Page! FreePages "<<GetFreePageNum()<<endl;
        return nullptr;
    }
    
    while(cur_order!=order)
    {
        Split(free_area[cur_order].head.next);
        cur_order--;
    }
   
    Page * cur_page = free_area[order].head.DismantleNext();
    free_page_num -= kpow2(order);
//    kout[Test]<<"Zone::AllocPage: "<<cur_page->index<<" "<<cur_page->order<<endl;
    cur_page->flags = 1;
    return cur_page;
}
void Zone::FreePage(Page * p)
{
	ISASBC;//??
	
//    kout[Test]<<"Zone::FreePage: "<<p<<" "<<p->index<<endl;
    free_area[p->order].head.AddNext(p);
    p->flags = 0;

    free_page_num += kpow2(p->order);
    Merge(p);
}

void Zone::Merge(Page * p)
{
	while (1)
	{
		Page * partner = GetPartner(p);
	    if(partner->flags != 0)//伙伴正在被使用或不可访问
	    {
	        return;
	    }
	    else
	    {
	//        kout[Test]<<"Zone::Merge: "<<p<<" "<<p->index<<endl;
	        p->Dismantle();
	        partner->Dismantle();
	        Page * parent = GetParent(p);
	        parent->flags = 0;
	        free_area[parent->order].head.AddNext(parent);
	        p=parent;
	    }
	}
}

void Zone::Split(Page * p)
{
//    kout[Test]<<"Zone::Split: "<<p<<" "<<p->index<<endl;
	p->flags=2;
    p->Dismantle();
    Page *lson = GetLeftSon(p),*rson = GetRightSon(p);
    int order = lson->order;
    free_area[order].head.AddNext(lson);
    free_area[order].head.AddNext(rson);
}

Page* Zone::GetPartner(Page * p)
{
    if(p->index%2==1)
    {
        return &page[p->index - 1];
    }
    else
    {
        return &page[p->index + 1];
    }
}
Page * Zone::GetLeftSon(Page * p)//获取左孩子
{
    return &page[p->index * 2];
}
Page * Zone::GetRightSon(Page * p)//获取右孩子
{
    return &page[p->index * 2+1];
}
Page * Zone::GetParent(Page * p)//获取父节点
{
    return &page[p->index / 2];
}
Page * Zone::QueryPage(int index,Uint64 addr)
{
    if(addr == page[index].addr&&page[index].flags == 1)
    {
        return &page[index];
    }
    if(page[index].order==0)
    {
        // kout<<"panic page not find"<<endl;
        return nullptr;
        //panic("can't find page");
    }
    Uint64 mid_addr = page[index].addr + kpow2(page[index].order-1)*page_size;
    if(addr<mid_addr)
    {
        return QueryPage(index*2,addr);
    }
    else
    {
        return QueryPage(index*2+1,addr);
    }
}
Page * Zone::GetPageFromAddr(void* addr)//根据物理地址和大小拿到对应的页
{   
    Page * p = QueryPage(1,(Uint64)addr);
    return p;
}

Uint64 Zone::GetFreePageNum()
{
    return free_page_num;
}


