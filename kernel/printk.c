#include "include/printk.h"
#include "include/sbi.h"
void Putchar(char ch)
{
	sbi_console_putchar(ch);
}

void Puts(const char *s)
{
	while (*s)
		Putchar(*s++);
}

template <typename T> void Swap(T &x,T &y)
{
	T t=x;
	x=y;
	y=t;
}

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
			dst[i++]=0;
	for (int j=(i>>1)-1;j>=0;--j)//??
		Swap(dst[j],dst[i-j-1]);
	return i;//\0 is not added automaticly!
}

class KOUT//Test
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
}kout;

KOUT& endl(KOUT &o)
{
	o<<"\n";
	return o;
}
void putchark(char ch) 
{
     sbi_console_putchar(ch);
}

void putsk(const char *str) 
{
    while (*str != '\0') 
    {
        putchark(*str);
        str++;
    }
}
void print_num(long long num,int base)
{
    /*
    num:要输出的数
    base:进制
    */
   kout<<num;
   if(num<0)
    {
         putchark('-');
         num = -num;
    }
    char buf[128];
    int len = 0;
    while(num)
    {
        buf[len++] = num % base + '0';
        num /= base;
    }
    buf[len] = '\0';
    for(int i=0;i<len/2;i++)//反转字符串
    {
        char tmp = buf[i];
        buf[i] = buf[len-1-i];
        buf[len-1-i] = tmp;
    }
    putsk(buf);
}
int printk_format(const char *  fmt,void *  sp) 
{
    /*fmt：格式化的字符串，例如:d,lld,.2f
    sp：当前参数地址
    返回值：sp应加的大小，如果不是完整的格式化字符串（例如一个'l'），应返回0，如果是不合法（例如'lll'），返回-1*/
    switch (fmt[1])
    {
        case 'd':
		{
            putsk("print int");
            int temp = *(int *)sp;
            print_num((long long)temp,10);
            return sizeof(int);  
		}
        case 'l':
		{
            switch (fmt[2])
            {
                case 'd':
                    /* code */
                    break;
                
                default:
                    break;
            }
		}
        default:
		{
            return -1;
		}
    }
    
}
void printk(const char *str,...)
{
    kout <<"a";
    void * sp = &str;
    sp += sizeof(void *);
    int format = -1;
    char formats[10];
    while (*str != '\0') 
    {
        if(format!=-1)
        {
            formats[++format] = *str;
            formats[format+1] = '\0';
            int size = printk_format(formats,sp);
            if(size>0)
            {
                format = -1;
                sp += size;
            }
            else if(size == -1)
            {
                format = -1;
            }
        }
        else if (*str == '%') 
        {
            format = 0;
        }
        else 
        {
            putchark(*str);
        }
        str++;
    }
}