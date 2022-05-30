#ifndef POS_FILEPATHTOOLS_HPP
#define POS_FILEPATHTOOLS_HPP

#include "../Library/String/StringTools.hpp"
#include "../Library/String/SysStringTools.hpp"
#include "../Library/DataStructure/PAL_Tuple.hpp"

namespace POS
{
	using namespace PAL_DS;
	
	inline char* GetLastSection(const char *path)
	{
		const char *s=path;
		while (*path)
			if (*path=='/')
				s=path=path+1;
			else ++path;
		return strDump(s);
	}
	
	inline char* GetPreviousSection(const char *path)
	{
		const char *last=path,
				   *p=path;
		while (*p)
			if (*p=='/')
				last=p;
			else ++p;
		if (last==path)
			return nullptr;
		char *re=(char*)Kmalloc(last-path+1);
		strCopy(re,path,last);
		re[last-path+1]=0;
		return re;
	}
	
	inline Doublet<char*,char*> CutLastSection(const char *path)
	{
		const char *last=path,
				   *p=path;
		while (*p)
		{
			if (*p=='/')
				last=p;
			++p;
		}
		Doublet <char*,char*> re(nullptr,nullptr);
		if (last!=path)
		{
			re.a=(char*)Kmalloc(last-path+1);
			strCopy(re.a,path,last);
			re.a[last-path+1]=0;
		}
		else re.a=strDump("/");
		++last;
		if (p!=last)
		{
			re.b=(char*)Kmalloc(p-last+1);
			strCopy(re.b,last,p);
			re.b[p-last+1]=0;
		}
		return re;
	}
};

#endif
