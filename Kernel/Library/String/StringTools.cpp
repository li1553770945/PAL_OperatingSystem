#include <Library/String/StringTools.hpp>

namespace POS
{
	void strCopy(char *dst,const char *src)
	{
		while (*src)
			*dst++=*src++;
		*dst=0;
	}
	
	void strCopy(char *dst,const char *src,const char *end)
	{
		while (src<end)
			*dst++=*src++;
		*dst=0;
	}
	
	char* strCopyRe(char *dst,const char *src)
	{
		while (*src)
			*dst++=*src++;
		return dst;
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
	
	int strComp(const char *s1,const char *se1,const char *s2)//??
	{
		while (s1<se1&&*s2&&*s1==*s2)
			++s1,++s2;
		if (s1==se1&&*s2==0)
			return 0;
		else if (s1==se1)
			return -1;
		else if (*s2==0)
			return 1;
		else if (*s1==*s2)
			return 0;
		else if (*s1<*s2)
			return -1;
		else return 1;
	}
	
	int strComp(const char *s1,const char *se1,const char *s2,const char *se2)//??
	{
		while (s1<se1&&s2<se2&&*s1==*s2)
			++s1,++s2;
		if (s1==se1&&s2==se2)
			return 0;
		else if (s1==se1)
			return -1;
		else if (s2==se2)
			return 1;
		else if (*s1==*s2)
			return 0;
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
	
	const char* strFind(const char *s,char ch)
	{
		while (*s)
			if (*s==ch)
				return s;
			else ++s;
		return nullptr;
	}
};
