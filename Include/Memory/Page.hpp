#ifndef PAGE_HPP
#define PAGE_HPP

#include <Memory/Const.hpp>
struct Page
{
	Page *pre,*next;
	int flags;//-1:不可使用，0未使用，1已使用
	int order;
	int index;
	Uint64 addr;
	
	Uint64 ref;//How many page entry referred to it.
	Page * DismantleNext();
	Page * Dismantle();
	void AddNext(Page * p);
	inline void* KAddr() const
	{return (void*)addr;}
	inline void* PAddr() const
	{return (void*)(addr-PhymemVirmemOffset());}
};

#endif