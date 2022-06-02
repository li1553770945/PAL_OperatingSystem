#ifndef POS_STRINGTOOLS_HPP
#define POS_STRINGTOOLS_HPP

namespace POS
{
	void strCopy(char *dst,const char *src);
	void strCopy(char *dst,const char *src,const char *end);
	char* strCopyRe(char *dst,const char *src);
	int strComp(const char *s1,const char *s2);
	int strComp(const char *s1,const char *se1,const char *s2);
	int strComp(const char *s1,const char *se1,const char *s2,const char *se2);
	unsigned long long strLen(const char *src);
	
	template <typename ...Ts> inline unsigned long long strLen(const char *src,const Ts *...others)
	{return strLen(src)+strLen(others...);}
	
	inline bool IsUpperCase(char ch)
	{return 'A'<=ch&&ch<='Z';}
	
	inline bool IsLowerCase(char ch)
	{return 'a'<=ch&&ch<='z';}
	
	inline bool IsLetter(char ch)
	{return IsUpperCase(ch)||IsLowerCase(ch);}
};

#endif
