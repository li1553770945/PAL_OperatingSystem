#ifndef POS_KOUT_HPP
#define POS_KOUT_HPP

#include "BasicFunctions.hpp"
#include "TemplateTools.hpp"
#include "String/Convert.hpp"
#include "../Error.hpp"

namespace POS
{
	namespace KoutEX
	{
		enum KoutEffect
		{
			Reset			=0,
			Bold			=1,
			Dim				=2,
			Underlined		=4,
			Blink			=5,
			Reverse			=7,
			Hidden			=8,
			ResetBold		=21,
			ResetDim		=22,
			ResetUnderlined	=24,
			ResetBlink		=25,
			ResetReverse	=27,
			ResetHidden		=28,
			
			ResetFore		=39,
			Black			=30,
			Red				=31,
			Green			=32,
			Yellow			=33,
			Blue			=34,
			Magenta			=35,
			Cyan			=36,
			LightGray		=37,
			DarkGray		=90,
			LightRed		=91,
			LightGreen		=92,
			LightYellow		=93,
			LightBlue		=94,
			LightMagenta	=95,
			LightCyan		=96,
			White			=97,
			
			ResetBG			=49,
			BlackBG			=40,
			RedBG			=41,
			GreenBG			=42,
			YellowBG		=43,
			BlueBG			=44,
			MagentaBG		=45,
			CyanBG			=46,
			LightGrayBG		=47,
			DarkGrayBG		=100,
			LightRedBG		=101,
			LightGreenBG	=102,
			LigthYellowBG	=103,
			LigthBlueBG		=104,
			LightMagentaBG	=105,
			LigthCyanBG		=106,
			WhiteBG			=107
		};
		
		enum KoutType
		{
			Info=0,
			Warning=1,
			Error=2,
			Debug=3,
			Fault=4,
			Test=5,
			
			NoneKoutType=31
		};
	};
	using namespace KoutEX;
	
	class KOUT
	{
		friend KOUT& endl(KOUT&);
		protected:
		public:
			const char *TypeName[32]{"Info","Warning","Error","Debug","Fault","Test",0};
			KoutEX::KoutEffect TypeColor[32]{KoutEX::Cyan,KoutEX::Yellow,KoutEX::Red,KoutEX::LightMagenta,KoutEX::LightRed,KoutEX::LightCyan,KoutEX::ResetFore};
			unsigned EnabledType=0xFFFFFFFF;
			unsigned CurrentType=31;
			unsigned RegisteredType=0;
			bool CurrentTypeOn=1,
				 EnableEffect=1;
			
			inline bool Precheck()
			{return CurrentTypeOn;}
			
			inline void SwitchCurrentType(unsigned p)
			{
				CurrentType=p;
				CurrentTypeOn=bool(1u<<p&EnabledType);
			}
			
		public:
			inline KOUT& operator [] (unsigned p)//Current Kout Info is type p. It will be clear when Endline is set!
			{
				ASSERT((p<=5||7<p&&p<=7+RegisteredType)&&p<=31,"KOUT: operator[p], p is not in valid range!");
				SwitchCurrentType(p);
				return *this<<Reset<<TypeColor[p]<<"["<<TypeName[p]<<"] ";
			}
			
			inline KOUT& operator [] (const char *type)
			{
				SwitchCurrentType(KoutEX::NoneKoutType);
				return *this<<Reset<<"["<<type<<"] ";
			}
			
			inline unsigned RegisterType(const char *name,KoutEX::KoutEffect color)
			{
				int re=0;
				if (RegisteredType<23)
				{
					re=++RegisteredType+7;
					TypeName[re]=name;
					TypeColor[re]=color;
				}
				return re;
			}
			
			inline void SwitchTypeOnoff(unsigned char type,bool onoff)
			{
				if (onoff)
					EnabledType|=1u<<type;
				else EnabledType&=~(1u<<type);
				SwitchCurrentType(NoneKoutType);
			}
			
			inline void SetEnabledType(unsigned en)
			{
				EnabledType=en;
				SwitchCurrentType(NoneKoutType);
			}
			
			inline bool GetTypeOnoff(unsigned char type)
			{return EnabledType&1<<type;}
			
			inline void SetEnableEffect(bool on)
			{EnableEffect=on;}
			
			inline KOUT& operator << (KoutEffect effect)
			{
				if (Precheck()&&EnableEffect)
					*this<<"\e["<<(int)effect<<"m";
				return *this;
			}
			
			inline KOUT& operator << (KOUT& (*func)(KOUT&))
			{
				return func(*this);
			}
			
			inline KOUT& operator << (const char *s)
			{
				if (Precheck())
					Puts(s);
				return *this;
			}
			
			inline KOUT& operator << (char *s)
			{return *this<<(const char*)s;}
			
			inline KOUT& operator << (bool b)
			{
				if (Precheck())
					Puts(b?"true":"false");
				return *this;
			}
			
			inline KOUT& operator << (char ch)
			{
				if (Precheck())
					Putchar(ch);
				return *this;
			}
			
			inline KOUT& operator << (unsigned char ch)
			{
				if (Precheck())
					Putchar(ch);
				return *this;
			}
			
			inline KOUT& operator << (unsigned long long x)
			{
				if (Precheck())
				{
					const int size=21;
					char buffer[size];
					int len=ullTOpadic(buffer,size,x,1,10);
					if (len!=-1)
					{
						buffer[len]=0;
						Puts(buffer);
					}
				}
				return *this;
			}
			
			inline KOUT& operator << (unsigned int x)
			{
				if (Precheck())
					operator << ((unsigned long long)x);
				return *this;
			}
			
			inline KOUT& operator << (unsigned short x)
			{
				if (Precheck())
					operator << ((unsigned long long)x);
				return *this;
			}
			
			inline KOUT& operator << (long long x)
			{
				if (Precheck())
				{
					if (x<0)
					{
						Putchar('-');
						if (x!=0x8000000000000000ull)//??
							x=-x;
					}
					operator << ((unsigned long long)x);
				}
				return *this;
			}
			
			inline KOUT& operator << (int x)
			{
				if (Precheck())
					operator << ((long long)x);
				return *this;
			}
			
			inline KOUT& operator << (short x)
			{
				if (Precheck())
					operator << ((long long)x);
				return *this;
			}
			
			template <typename T> KOUT& operator << (T *p)
			{
				if (Precheck())
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
				}
				return *this;
			}
			
			template <typename T,typename ...Ts> KOUT& operator << (T(*p)(Ts...))
			{
				if (Precheck())
					operator << ((void*)p);
				return *this;
			}
			
			inline KOUT& operator << (const DataWithSize &x)
			{
				if (Precheck())
				{
					const int size=3;
					char buffer[size];
					for (unsigned long long i=0;i<x.size;++i)
					{
						int len=ullTOpadic(buffer,size,*((char*)x.data+i),1,16,2);
						if (len!=-1)
						{
							buffer[len]=0;
							Puts(buffer);
							Putchar(' ');
						}
					}
				}
				return *this;
			}
			
			inline KOUT& operator << (const DataWithSizeUnited &x)
			{
				if (Precheck())
				{
					for (Uint64 s=0,i=0;s<x.size;s+=x.unitSize,++i)
						*this<<i<<": "<<DataWithSize(x.data+s,x.unitSize)<<"\n";
				}
				return *this;
			}
			
			template <typename T> KOUT& operator << (const T &x)
			{
				if (Precheck())
				{
					Puts("Unknown KOUT type:");
					const int size=3;
					char buffer[size];
					for (int i=0;i<sizeof(x);++i)
					{
						int len=ullTOpadic(buffer,size,*((char*)&x+i),1,16,2);
						if (len!=-1)
						{
							buffer[len]=0;
							Puts(buffer);
							Putchar(' ');
						}
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
				if (Precheck())
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
			
			void Init()
			{
				
			}
	};
	
	extern KOUT kout;
	
	inline KOUT& endl(KOUT &o)
	{
		o<<"\n"<<Reset;
		if (o.CurrentType==KoutEX::Fault)
		{
			o.SwitchCurrentType(KoutEX::NoneKoutType);
			KernelFaultSolver();
		}
		o.SwitchCurrentType(KoutEX::NoneKoutType);
		return o;
	}
	
	inline KOUT& endline(KOUT &o)
	{return o<<"\n";}
};

#define ASSERTEX(x,s)						\
{											\
	if (!(x))								\
		POS::kout[POS::Fault]<<s<<POS::endl;\
}

#endif
