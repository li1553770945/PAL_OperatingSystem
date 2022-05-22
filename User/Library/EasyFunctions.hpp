#ifndef EASYFUNCTIONS_HPP
#define EASYFUNCTIONS_HPP

#include <Library/TemplateTools.hpp>
using namespace POS;

inline void puts(const char *s)
{
	if (s==nullptr)//??
		return;
	while (*s)
		Sys_Putchar(*s++);
}

inline void Loop(long long t)
{while (t-->0);}

int ullTOpadic(char *dst,unsigned size,unsigned long long x,bool isA=1,unsigned char p=16,unsigned w=1)
{
	if (dst==nullptr||size==0||p>=36||w>size)//<<Precheck, change it to assert later...
		return 0;
	unsigned i=0;
	while (x&&i<size)
	{
		int t=x%p;
		if (t<=9) t+='0';
		else t+=(isA?'A':'a')-10;
		dst[i++]=t;
		x/=p;
	}
	if (i==size&&x)
		return -1;
	if (w!=0)
		while (i<w)
			dst[i++]='0';
	for (int j=(i>>1)-1;j>=0;--j)//??
		Swap(dst[j],dst[i-j-1]);
	return i;//\0 is not added automaticly!
}

class COUT
{
	public:
		inline COUT& operator << (COUT& (*func)(COUT&))
		{
			return (*func)(*this);
		}
		
		inline COUT& operator << (const char *s)
		{
			puts(s);
			return *this;
		}
		
		inline COUT& operator << (char ch)
		{
			Sys_Putchar(ch);
			return *this;
		}
		
		inline COUT& operator << (unsigned char ch)
		{
			Sys_Putchar(ch);
			return *this;
		}
		
		inline COUT& operator << (unsigned long long x)
		{
			const int size=21;
			char buffer[size];
			int len=ullTOpadic(buffer,size,x,1,10);
			if (len!=-1)
			{
				buffer[len]=0;
				puts(buffer);
			}
			return *this;
		}
		
		inline COUT& operator << (unsigned int x)
		{
			operator << ((unsigned long long)x);
			return *this;
		}
		
		inline COUT& operator << (unsigned short x)
		{
			operator << ((unsigned long long)x);
			return *this;
		}
		
		inline COUT& operator << (long long x)
		{
			if (x<0)
			{
				Sys_Putchar('-');
				if (x!=0x8000000000000000ull)//??
					x=-x;
			}
			operator << ((unsigned long long)x);
			return *this;
		}
		
		inline COUT& operator << (int x)
		{
			operator << ((long long)x);
			return *this;
		}
		
		inline COUT& operator << (short x)
		{
			operator << ((long long)x);
			return *this;
		}
		
		template <typename T> COUT& operator << (T *p)
		{
			Sys_Putchar('0');
			Sys_Putchar('x');
			const int size=17;//??
			char buffer[size];
			int len=ullTOpadic(buffer,size,(unsigned long long)p);
			if (len!=-1)
			{
				buffer[len]=0;
				puts(buffer);
			}
			return *this;
		}
		
		template <typename T,typename ...Ts> COUT& operator << (T(*p)(Ts...))
		{
			operator << ((void*)p);
			return *this;
		}
		
		inline COUT& operator << (const DataWithSize &x)
		{
			puts("Data:");
			const int size=3;
			char buffer[size];
			for (unsigned long long i=0;i<x.size;++i)
			{
				int len=ullTOpadic(buffer,size,*((char*)x.data+i));
				if (len!=-1)
				{
					buffer[len]=0;
					puts(buffer);
				}
			}
			return *this;
		}
		
		template <typename T> COUT& operator << (const T &x)
		{
			puts("Unknown COUT type:");
			const int size=3;
			char buffer[size];
			for (int i=0;i<sizeof(x);++i)
			{
				int len=ullTOpadic(buffer,size,*((char*)&x+i));
				if (len!=-1)
				{
					buffer[len]=0;
					puts(buffer);
				}
			}
			return *this;
		}
	
	protected:
		template <typename T> void PrintFirst(const char *s,const T &x)
		{
			operator << (x);
			puts(s);
		}
		
		template <typename T,typename ...Ts> void PrintFirst(const char *s,const T &x,const Ts &...args)
		{
			operator << (x);
			operator () (s,args...);
		}
		
	public:
		template <typename ...Ts> COUT& operator () (const char *s,const Ts &...args)
		{
			while (*s)
				if (*s!='%')
					Sys_Putchar(*s),++s;
				else if (*(s+1)=='%')
					Sys_Putchar('%'),s+=2;
				else if (InRange(*(s+1),'a','z')||InRange(*(s+1),'A','Z'))
				{
					PrintFirst(s+2,args...);
					return *this;
				}
				else Sys_Putchar(*s),++s;
			return *this;
		}
}cout;

static inline COUT& endl(COUT &o)
{
	o<<"\n";
	return o;
}

#endif
