#ifndef POS_TEMPLATETOOLS_HPP
#define POS_TEMPLATETOOLS_HPP

#include "../Types.hpp"

namespace POS
{
	template <typename T> inline void Swap(T &x,T &y)
	{
		T t=x;
		x=y;
		y=t;
	}
	
	template <typename T1,typename T2,typename T3> inline bool InRange(const T1 &x,const T2 &L,const T3 &R)
	{return L<=x&&x<=R;}
	
	template <typename T1,typename T2,typename T3> inline T1 EnsureInRange(const T1 &x,const T2 &L,const T3 &R)
	{
		if (x<L) return L;
		else if (x>R) return R;
		else return x;
	}
	
	template <typename T0,typename T1> inline bool InThisSet(const T0 &x,const T1 &a)
	{return x==a;}
	
	template <typename T0,typename T1,typename...Ts> inline bool InThisSet(const T0 &x,const T1 &a,const Ts &...args)
	{
		if (x==a) return 1;
		else return InThisSet(x,args...);
	}
	
	template <typename T0,typename T1> inline bool NotInSet(const T0 &x,const T1 &a)
	{return x!=a;}
	
	template <typename T0,typename T1,typename...Ts> inline bool NotInSet(const T0 &x,const T1 &a,const Ts &...args)
	{
		if (x==a) return 0;
		else return NotInSet(x,args...);
	}
	
	template <typename T> inline T max3(const T &x,const T &y,const T &z)
	{
		if (x<y)
			if (y<z) return z;
			else return y;
		else 
			if (x<z) return z;
			else return x;
	}
	
	template <typename T> inline T min3(const T &x,const T &y,const T &z)
	{
		if (x<y)
			if (x<z) return x;
			else return z;
		else
			if (y<z) return y;
			else return z;
	}
	
	template <typename T> inline T maxN(const T &a,const T &b)
	{return a<b?b:a;}
	
	template <typename T,typename...Ts> inline T maxN(const T &a,const T &b,const Ts &...args)
	{
		if (a<b) return (T)maxN(b,(T)args...);
		else return (T)maxN(a,(T)args...);
	}
	
	template <typename T> inline T minN(const T &a,const T &b)
	{return b<a?b:a;}
	
	template <typename T,typename...Ts> inline T minN(const T &a,const T &b,const Ts &...args)
	{
		if (b<a) return (T)minN(b,(T)args...);
		else return (T)minN(a,(T)args...);
	}
	
//	template <typename T> inline T*& DeleteToNULL(T *&ptr)
//	{
//		if (ptr!=nullptr)
//		{
//			delete ptr;
//			ptr=nullptr;
//		}
//		return ptr;
//	}
//	
//	template <typename T> inline T*& DELETEtoNULL(T *&ptr)
//	{
//		if (ptr!=nullptr)
//		{
//			delete[] ptr;
//			ptr=nullptr;
//		}
//		return ptr;
//	}
	
	template <typename T> void MemsetT(T *dst,const T &val,unsigned long long count)
	{
		if (dst==nullptr||count==0) return;
		T *end=dst+count;
		while (dst!=end)
			*dst++=val;
	}
	
	template <typename T> void MemcpyT(T *dst,const T *src,unsigned long long count)
	{
		if (dst==nullptr||src==nullptr||count==0) return;
		T *end=dst+count;
		while (dst!=end)
			*dst++=*src++;
	}
	
	template <typename T> T* OperateForAll(T *src,unsigned long long count,void(*func)(T&))
	{
		if (src==nullptr) return nullptr;
		for (unsigned long long i=0;i<count;++i)
			func(src[i]);
		return src;
	}
	
	class BaseTypeFuncAndData
	{
		public:
			virtual int CallFunc(int usercode)=0;
			virtual ~BaseTypeFuncAndData() {};
	};
	
	template <class T> class TypeFuncAndData:public BaseTypeFuncAndData
	{
		protected:
			int (*func)(T&,int)=nullptr;
			T funcdata;
		public:
			virtual int CallFunc(int usercode)
			{
				if (func!=nullptr)
					return func(funcdata,usercode);
				else return 0;
			}
			
			virtual ~TypeFuncAndData() {}
			
			TypeFuncAndData(int (*_func)(T&,int),const T &_funcdata):func(_func),funcdata(_funcdata) {}
	};
	
	template <class T> class TypeFuncAndDataV:public BaseTypeFuncAndData
	{
		protected:
			void (*func)(T&)=nullptr;
			T funcdata;
		public:
			virtual int CallFunc(int usercode)
			{
				if (func!=nullptr)
					func(funcdata);
				return 0;
			}
			
			virtual ~TypeFuncAndDataV() {}
			
			TypeFuncAndDataV(void (*_func)(T&),const T &_funcdata):func(_func),funcdata(_funcdata) {}
	};
	
	class VoidFuncAndData:public BaseTypeFuncAndData
	{
		protected:
			void (*func)(void*)=nullptr;
			void *funcdata=nullptr;
		public:
			virtual int CallFunc(int)
			{
				if (func!=nullptr)
					func(funcdata);
				return 0;
			}
			
			VoidFuncAndData(void (*_func)(void*),void *_funcdata=nullptr):func(_func),funcdata(_funcdata) {}
	};
	
	class DataWithSize
	{
		public:
			void *data;
			Uint64 size;
			
			DataWithSize(void *_data,Uint64 _size):data(_data),size(_size) {}
	};
	
	class DataWithSizeUnited:public DataWithSize
	{
		public:
			Uint64 unitSize;
			
			DataWithSizeUnited(void *_data,Uint64 _size,Uint64 _unitsize):DataWithSize(_data,_size),unitSize(_unitsize) {}
	};
	
	template <typename T> inline bool GetBitMask(T tar,unsigned i)
	{return (tar>>i)&1;}
	
	template <typename T> inline void SetBitMask1(T &tar,unsigned i)
	{tar|=1ull<<i;}
	
	template <typename T> inline void SetBitMask0(T &tar,unsigned i)
	{tar&=~(1ull<<i);}
};

#endif
