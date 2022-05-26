#ifndef POS_SYSSTRINGTOOLS_HPP
#define POS_SYSSTRINGTOOLS_HPP

#include "StringTools.hpp"
#include "../../Memory/PhysicalMemory.hpp"

namespace POS
{
	inline char* strDump(const char *src)
	{
		auto len=strLen(src);
		if (len==0)
			return nullptr;
		char *re=(char*)Kmalloc(len+1);
		strCopy(re,src);
		return re;
	}

	inline char *strSplice(const char *a,const char *b)
	{
		auto lenA=strLen(a);
		auto lenB=strLen(b);
		char *re=(char*)Kmalloc(lenA+lenB+1);
		strCopy(re,a);
		strCopy(re+lenA,b);
		return re;
	}
	
	inline void _strSplice(char *dst,const char *src)
	{strCopy(dst,src);}
	
	template <typename ...Ts> inline void _strSplice(char *dst,const char *src,const Ts *...others)
	{
		dst=strCopyRe(dst,src);
		_strSplice(dst,others...);
	}
	
	template <typename ...Ts> inline char* strSplice(const Ts *...srcs)
	{
		auto len=strLen(srcs...);
		char *re=(char*)Kmalloc(len+1);
		_strSplice(re,srcs...);
		return re;
	}
};

#endif
