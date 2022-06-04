extern "C"
{
	#include <HAL/Drivers/DriverTools.h>
	
	void set_bit(volatile uint32 *bits, uint32 mask, uint32 value)
	{
	    uint32 org = (*bits) & ~mask;
	    *bits = org | (value & mask);
	}
	
	void set_bit_offset(volatile uint32 *bits, uint32 mask, uint64 offset, uint32 value)
	{
	    set_bit(bits, mask << offset, value << offset);
	}
	
	void set_gpio_bit(volatile uint32 *bits, uint64 offset, uint32 value)
	{
	    set_bit_offset(bits, 1, offset, value);
	}
	
	uint32 get_bit(volatile uint32 *bits, uint32 mask, uint64 offset)
	{
	    return ((*bits) & (mask << offset)) >> offset;
	}
	
	uint32 get_gpio_bit(volatile uint32 *bits, uint64 offset)
	{
	    return get_bit(bits, 1, offset);
	}
};
#include <Process/Synchronize.hpp>
#include <Library/TemplateTools.hpp>
using namespace POS;

void* kalloc()
{
	return Kmalloc(4096/*???*/);
}

void kfree(void *addr)
{
	Kfree(addr);
}

void* memmove(void *dst, const void *src,unsigned long long size)
{
	MemmoveT<char>((char*)dst,(char *)src,size);
	return dst;
}

Semaphore *dmacSem=nullptr;
void dmacSemWait()
{
//	dmacSem->Wait();
}

void dmacSemSignal()
{
//	dmacSem->Signal(Semaphore::SignalAll);
}

void dmacSemInit()
{
	dmacSem=new Semaphore(0);
}

void DebugInfo(const char *str)
{
	kout[Debug]<<str<<endl;
}

void DebugInfoI(const char *str,unsigned long long x)
{
	kout[Debug]<<str<<x<<endl;
}
