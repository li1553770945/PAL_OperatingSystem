#ifndef POS_FILEPATHTOOLS_HPP
#define POS_FILEPATHTOOLS_HPP

#include "../Library/String/StringTools.hpp"
#include "../Library/String/SysStringTools.hpp"
#include "../Library/DataStructure/PAL_Tuple.hpp"

namespace POS
{
	using namespace PAL_DS;
	
	inline const char* FindFirstSection(const char *path,bool includeFirst=0)
	{
		if (path==nullptr)
			return nullptr;
		if (!includeFirst)
			++path;
		while (*path)
			if (*path=='/')
				break;
			else ++path;
		return path;
	}
	
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
		re[last-path]=0;
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
			re.a[last-path]=0;
		}
		else re.a=strDump("/");
		++last;
		if (p!=last)
		{
			re.b=(char*)Kmalloc(p-last+1);
			strCopy(re.b,last,p);
			re.b[p-last]=0;
		}
		return re;
	}
	
	inline Uint64 UnicodeToUtf8(char *dst,Uint32 unicode[],Uint32 len)
	{
		Uint64 char_len = 0;
		for (Uint32 i = 0; i < len; i++)
		{
			if (unicode[i] <= 0x7F) {
				// Plain ASCII
				dst[char_len++] = (char)unicode[i];
			}
			else if (unicode[i] <= 0x07FF) {
				// 2-byte unicode
				dst[char_len++] = (char)(((unicode[i] >> 6) & 0x1F) | 0xC0);
				dst[char_len++] = (char)(((unicode[i] >> 0) & 0x3F) | 0x80);
			}
			else if (unicode[i] <= 0xFFFF) {
				// 3-byte unicode
				dst[char_len++] = (char)(((unicode[i] >> 12) & 0x0F) | 0xE0);
				dst[char_len++] = (char)(((unicode[i] >> 6) & 0x3F) | 0x80);
				dst[char_len++] = (char)(((unicode[i] >> 0) & 0x3F) | 0x80);
			}
			else if (unicode[i] <= 0x10FFFF) {
				// 4-byte unicode
				dst[char_len++] = (char)(((unicode[i] >> 18) & 0x07) | 0xF0);
				dst[char_len++] = (char)(((unicode[i] >> 12) & 0x3F) | 0x80);
				dst[char_len++] = (char)(((unicode[i] >> 6) & 0x3F) | 0x80);
				dst[char_len++] = (char)(((unicode[i] >> 0) & 0x3F) | 0x80);
			}
			else {
				// error 
				kout[Fault] << "UnicodeToUtf8 fault!"<<endl;
				return 0;
			}
		}
		dst[char_len] = 0;
		return char_len;
	}
	
	inline PAL_DS::Doublet<Uint32*, Uint32> Utf8ToUnicode(const char * name)
	{
		Uint32* unicode = new Uint32[strLen(name)];
		Uint32 unicode_len = 0;
		int read_pos = 0;

		while(read_pos < strLen(name))
		{
			unsigned char mask = 1 << 7;
			int size = 0;
			for (; size < 8; size++)
			{
				if ( (name[read_pos] & mask) == 0)
					break;
				mask >>= 1;
			}
			switch (size)
			{
				case 0:
					unicode[unicode_len] = (Uint32)name[read_pos];
					unicode_len++;
					read_pos++;
					break;
				default:
					kout[Fault] << "we have not support utf8 more than one size!"<<endl;
					break;
			}
		}
		return PAL_DS::Doublet<Uint32*, Uint32>(unicode,unicode_len);
	}
};

#endif
