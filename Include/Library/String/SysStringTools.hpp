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
};

#endif
