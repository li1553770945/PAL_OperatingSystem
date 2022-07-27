#ifndef POS_STRING_HPP
#define POS_STRING_HPP

#include "StringTools.hpp"
#include "SysStringTools.hpp"

void Assert(bool,const char*);

namespace POS
{
//	class ConstString
//	{
//		protected:
//			char 
//			
//		public:
//			
//			
//	};
	
	class String
	{
		public:
			enum class Flag
			{
				MoveFrom=1
			};
		
		protected:
			char *s=nullptr;
			
		public:
			const char* cStr() const
			{return s;}
			
			String operator + (const String &src) const
			{return String(strSplice(s,src.s),Flag::MoveFrom);}
			
			String& operator += (const String &src)
			{
				char *a=s;
				s=strSplice(s,src.s);
				delete[] s;
				return *this;
			}
			
			~String()
			{
				if (s)
					delete[] s;
			}
			
			String(char *str,Flag flag)
			{
				if (flag==Flag::MoveFrom)
					s=str;
			}
			
			String(const char *str)
			{
				if (str!=nullptr)
				{
					s=strDump(str);
					Assert(s!=nullptr,"Failed to construct String!");
				}
			}
			
			String() {}
	};
	
};

#endif
