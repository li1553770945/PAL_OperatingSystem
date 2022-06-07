#ifndef BUDDY_HPP
#define BUDDY_HPP

#include <Types.hpp>
#include <Memory/Page.hpp>

class FreeArea
{
	public:
		Page head;
};

class Zone
{
	public:
		Uint64 physical_memory_size;
		Uint64 end_addr;//物理内存结束地址

		Uint64 page_size;//每个页的大小
		Uint64 page_base;//内核结束地址，也就是页表的起始地址
		Page * page;

		Uint64 page_num;//页的数量
		Uint64 valid_page_num;//实际可用的页的数量

		Uint64 page_need_memory;//页表需要的内存

		Uint64 free_memory_start_addr;//除去页表，实际可用内存的起始地址
		Uint64 free_memory_size;//除去页表，实际可用内存的大小
		/*
		|----kernel---------------|------------------page-------------|---------------free_memory-----------|
		-------------kernel_end(page_base)------------------free_memmory_start---------------------------end_addr
		*/
	
		int max_order;
		int max_depth;
		FreeArea  free_area[64];
		
		int Init();//初始化
		Page* AllocPage(Uint64 need_page_num);//需要2^block个页
		void FreePage(Page *);//释放
		void InitTree(int index,int order,int left,int right);//初始化所有节点为不可使用
		void BuildTree(int pos,int left,int right,int x,int y,Uint64 addr);//构建树
		void InitArea(int pos,int left,int right);//初始化每个order的头结点
		void Split(Page* P);//分裂
		void Merge(Page* P);//合并
		Page * GetLeftSon(Page* P);//获取左孩子
		Page * GetRightSon(Page* P);//获取右孩子
		Page * GetParent(Page* P);//获取父节点
		Page * GetPartner(Page * p);//获取伙伴
		Page * GetPageFromAddr(void*);//根据物理地址和大小拿到对应的页
		Page * QueryPage(int index,Uint64 addr);//递归查找地址对应的页
};

#endif