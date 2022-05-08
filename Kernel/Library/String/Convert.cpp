#include <Library/String/Convert.hpp>
#include <Library/TemplateTools.hpp>

namespace POS
{
	int ullTOpadic(char *dst,unsigned size,unsigned long long x,bool isA,unsigned char p,unsigned w)
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
};
