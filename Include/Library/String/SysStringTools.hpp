#ifndef POS_SYSSTRINGTOOLS_HPP
#define POS_SYSSTRINGTOOLS_HPP

#include "StringTools.hpp"
#include "../DataStructure/PAL_Tuple.hpp"

namespace POS
{
	inline char* strDump(const char *src)
	{
		auto len=strLen(src);
		if (len==0)
			return nullptr;
		char *re=new char[len+1];
		strCopy(re,src);
		return re;
	}
	
	inline char *strDump(const char *src,const char *end)
	{
		long long len=end-src;
		if (len<=0)
			return nullptr;
		char *re=new char[len+1];
		strCopy(re,src,end);
		return re;
	}

	inline char *strSplice(const char *a,const char *b)
	{
		auto lenA=strLen(a);
		auto lenB=strLen(b);
		char *re=new char[lenA+lenB+1];
		strCopy(re,a);
		strCopy(re+lenA,b);
		return re;
	}
	
	inline PAL_DS::Doublet <Uint32,char**> divideStringByChar(const char *src,char ch,bool skipEmpty=0)
	{
		if (src==nullptr)
			return {0,nullptr};
		const char *s=src;
		unsigned cnt=0;
		bool flag=0;
		while (1)
		{
			if (flag==0&&!(skipEmpty&&(*s==ch||*s==0)))
				++cnt,flag=1;
			if (*s==0)
				break;
			else if (*s==ch)
				flag=0;
			++s;
		}
		char **re=new char*[cnt];
		const char *p=s=src;
		int i=0;
		flag=0;
		while (1)
		{
			if (flag==0&&!(skipEmpty&&(*p==ch||*p==0)))
				flag=1;
			if (*p==0)
			{
				if (flag)
				{
					re[i++]=strDump(s,p);
					s=++p;
				}
				break;
			}
			else if (*p==ch)
			{
				if (flag)
					re[i++]=strDump(s,p);
				s=++p;
				flag=0;
			}
			else ++p;
		}
		return {cnt,re};
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
		char *re=new char[len+1];
		_strSplice(re,srcs...);
		return re;
	}
};

#endif
