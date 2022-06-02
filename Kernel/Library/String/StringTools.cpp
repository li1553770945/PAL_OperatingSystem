#include <Library/String/StringTools.hpp>
#include <Library/Kout.hpp>
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
	Uint64 UnicodeToUtf8(char* out, Uint32 unicode[], Uint32 unicode_len)
	{
		Uint64 char_len = 0;
		for (Uint32 i = 0; i < unicode_len; i++)
		{
			if (unicode[i] <= 0x7F) {
				// Plain ASCII
				out[char_len++] = (char)unicode[i];
			}
			else if (unicode[i] <= 0x07FF) {
				// 2-byte unicode
				out[char_len++] = (char)(((unicode[i] >> 6) & 0x1F) | 0xC0);
				out[char_len++] = (char)(((unicode[i] >> 0) & 0x3F) | 0x80);
			}
			else if (unicode[i] <= 0xFFFF) {
				// 3-byte unicode
				out[char_len++] = (char)(((unicode[i] >> 12) & 0x0F) | 0xE0);
				out[char_len++] = (char)(((unicode[i] >> 6) & 0x3F) | 0x80);
				out[char_len++] = (char)(((unicode[i] >> 0) & 0x3F) | 0x80);
			}
			else if (unicode[i] <= 0x10FFFF) {
				// 4-byte unicode
				out[char_len++] = (char)(((unicode[i] >> 18) & 0x07) | 0xF0);
				out[char_len++] = (char)(((unicode[i] >> 12) & 0x3F) | 0x80);
				out[char_len++] = (char)(((unicode[i] >> 6) & 0x3F) | 0x80);
				out[char_len++] = (char)(((unicode[i] >> 0) & 0x3F) | 0x80);
			}
			else {
				// error 
				return 0;
			}
		}
		out[char_len] = 0;
		return char_len;
	}

	PAL_DS::Doublet<Uint32*, Uint32> Utf8ToUnicode(const char * name)
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
				{
					break;
				}
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
					kout[Fault] << "we have not support utf8 more than one size!";
					break;
			}


		}
		
		return PAL_DS::Doublet<Uint32*, Uint32>(unicode,unicode_len);
	}
	
};
