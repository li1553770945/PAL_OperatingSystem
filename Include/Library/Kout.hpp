#ifndef POS_KOUT_HPP
#define POS_KOUT_HPP

#include "BasicFunctions.hpp"
#include "TemplateTools.hpp"
#include "String/Convert.hpp"

namespace POS
{
	class KOUT
	{
		public:
			inline KOUT& operator << (KOUT& (*func)(KOUT&))
			{
				return (*func)(*this);
			}
			
			inline KOUT& operator << (const char *s)
			{
				Puts(s);
				return *this;
			}
			
			inline KOUT& operator << (char ch)
			{
				Putchar(ch);
				return *this;
			}
			
			inline KOUT& operator << (unsigned char ch)
			{
				Putchar(ch);
				return *this;
			}
			
			inline KOUT& operator << (unsigned long long x)
			{
				const int size=21;
				char buffer[size];
				int len=ullTOpadic(buffer,size,x,1,10);
				if (len!=-1)
				{
					buffer[len]=0;
					Puts(buffer);
				}
				return *this;
			}
			
			inline KOUT& operator << (unsigned int x)
			{
				operator << ((unsigned long long)x);
				return *this;
			}
			
			inline KOUT& operator << (unsigned short x)
			{
				operator << ((unsigned long long)x);
				return *this;
			}
			
			inline KOUT& operator << (long long x)
			{
				if (x<0)
				{
					Putchar('-');
					if (x!=0x8000000000000000ull)//??
						x=-x;
				}
				operator << ((unsigned long long)x);
				return *this;
			}
			
			inline KOUT& operator << (int x)
			{
				operator << ((long long)x);
				return *this;
			}
			
			inline KOUT& operator << (short x)
			{
				operator << ((long long)x);
				return *this;
			}
			
			template <typename T> KOUT& operator << (T *p)
			{
				Putchar('0');
				Putchar('x');
				const int size=17;//??
				char buffer[size];
				int len=ullTOpadic(buffer,size,(unsigned long long)p);
				if (len!=-1)
				{
					buffer[len]=0;
					Puts(buffer);
				}
				return *this;
			}
			
			template <typename T,typename ...Ts> KOUT& operator << (T(*p)(Ts...))
			{
				operator << ((void*)p);
				return *this;
			}
			
			template <typename T> KOUT& operator << (const T &x)
			{
				Puts("Unknown KOUT type:");
				for (int i=0;i<sizeof(x);++i)
				{
					const int size=3;
					char buffer[size];
					int len=ullTOpadic(buffer,size,*((char*)&x+i));
					if (len!=-1)
					{
						buffer[len]=0;
						Puts(buffer);
					}
				}
				return *this;
			}
		
		protected:
			template <typename T> void PrintFirst(const char *s,const T &x)
			{
				operator << (x);
				Puts(s);
			}
			
			template <typename T,typename ...Ts> void PrintFirst(const char *s,const T &x,const Ts &...args)
			{
				operator << (x);
				operator () (s,args...);
			}
			
		public:
			template <typename ...Ts> KOUT& operator () (const char *s,const Ts &...args)
			{
				while (*s)
					if (*s!='%')
						Putchar(*s),++s;
					else if (*(s+1)=='%')
						Putchar('%'),s+=2;
					else if (InRange(*(s+1),'a','z')||InRange(*(s+1),'A','Z'))
					{
						PrintFirst(s+2,args...);
						return *this;
					}
					else Putchar(*s),++s;
				return *this;
			}
	};
	
	extern KOUT kout;
	
	static inline KOUT& endl(KOUT &o)
	{
		o<<"\n";
		return o;
	}
};

#endif
