#include <Library/String/StringTools.hpp>

namespace POS
{
	void strCopy(char *dst,const char *src)
	{
		while (*src)
			*dst++=*src++;
		*dst=0;
	}
};
