#include <Library/TemplateTools.hpp>
#include <Memory/PhysicalMemory.hpp>
using namespace POS;

using size_t=long unsigned int;

extern "C"
{
//	void *__dso_handle=0;
//	void *__cxa_atexit=0;
//	void *_Unwind_Resume=0;
//	void *__gxx_personality_v0=0;
	void *__cxa_pure_virtual=0;
//????
	
	void* memset(void *_dst,int _Val,size_t _Size)
	{
		MemsetT<char>((char*)_dst,_Val,_Size);
		return _dst;
	}
};

void* operator new(size_t size)
{return Kmalloc(size);}

void* operator new[](size_t size)
{return Kmalloc(size);}

void operator delete(void *p,size_t size)
{Kfree(p);}

void operator delete[](void *p)
{Kfree(p);}
