#include <Library/String/StringTools.hpp>
#include <Types.hpp>
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

	Uint64 UnicodeToUtf8(char* out, Uint32 utf[], Uint32 int_len)
	{
		Uint64 char_len = 0;
		for (int i = 0; i < int_len; i++)
		{
			if (utf[i] <= 0x7F) {
				// Plain ASCII
				out[char_len++] = (char)utf[i];
			}
			else if (utf[i] <= 0x07FF) {
				// 2-byte unicode
				out[char_len++] = (char)(((utf[i] >> 6) & 0x1F) | 0xC0);
				out[char_len++] = (char)(((utf[i] >> 0) & 0x3F) | 0x80);
			}
			else if (utf[i] <= 0xFFFF) {
				// 3-byte unicode
				out[char_len++] = (char)(((utf[i] >> 12) & 0x0F) | 0xE0);
				out[char_len++] = (char)(((utf[i] >> 6) & 0x3F) | 0x80);
				out[char_len++] = (char)(((utf[i] >> 0) & 0x3F) | 0x80);
			}
			else if (utf[i] <= 0x10FFFF) {
				// 4-byte unicode
				out[char_len++] = (char)(((utf[i] >> 18) & 0x07) | 0xF0);
				out[char_len++] = (char)(((utf[i] >> 12) & 0x3F) | 0x80);
				out[char_len++] = (char)(((utf[i] >> 6) & 0x3F) | 0x80);
				out[char_len++] = (char)(((utf[i] >> 0) & 0x3F) | 0x80);
			}
			else {
				// error 
				return 0;
			}
		}
		out[char_len] = 0;
		return char_len;
	}
};
