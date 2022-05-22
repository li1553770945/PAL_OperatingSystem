#include <Library/String/StringTools.hpp>

namespace POS
{
	void strCopy(char *dst,const char *src)
	{
		while (*src)
			*dst++=*src++;
		*dst=0;
	}
	
	int strComp(const char *s1,const char *s2)
	{
		while (*s1&&*s1==*s2)
			++s1,++s2;
		if (*s1==*s2)
			return 0;
		else if (*s1==0)
			return -1;
		else if (*s2==0)
			return 1;
		else if (*s1<*s2)
			return -1;
		else return 1;
	}
	
	unsigned long long strLen(const char *src)
	{
		if (src==nullptr) return 0;
		unsigned long long re=0;
		while (src[re]!=0)
			++re;
		return re;
	}
};
