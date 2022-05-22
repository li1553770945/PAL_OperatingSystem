#ifndef POS_LINKTABLE_HPP
#define POS_LINKTABLE_HPP

#include "../../Error.hpp"

namespace POS
{
	template <class T> class LinkTableT//Need extra node as head node;
	{
		protected:
			T *pre,
			  *nxt;
			
		public:
			inline T* This()//???
			{return (T*)this;}
			
			inline void PreInsert(T *p)
			{
				ASSERT(p->Single(),"LinkTableT:PreInsert:p is not Single!");
				if (pre==nullptr)
				{
					p->nxt=This();
					pre=p;
				}
				else
				{
					p->pre=pre;
					p->nxt=This();
					pre->nxt=p;
					pre=p;
				}
			}
			
			inline void NxtInsert(T *p)
			{
				ASSERT(p->Single(),"LinkTable:NxtInsert:p is not Single!");
				if (nxt==nullptr)
				{
					p->pre=This();
					nxt=p;
				}
				else
				{
					p->pre=This();
					p->nxt=nxt;
					nxt->pre=p;
					nxt=p;
				}
			}
			
			inline void Remove()
			{
				if (pre==nullptr&&nxt==nullptr)
					DoNothing;
				else if (pre==nullptr)
				{
					nxt->pre=nullptr;
					nxt=nullptr;
				}
				else if (nxt==nullptr)
				{
					pre->nxt=nullptr;
					pre=nullptr;
				}
				else
				{
					pre->nxt=nxt;
					nxt->pre=pre;
					pre=nxt=nullptr;
				}
			}
			
			inline bool Single() const
			{return pre==nullptr&&nxt==nullptr;}
			
			inline bool PreEmpty() const
			{return pre==nullptr;}
			
			inline bool NxtEmpty() const
			{return nxt==nullptr;}
			
			inline T* Pre()
			{return pre;}
			
			inline T* Nxt()
			{return nxt;}
			
			inline void Init()
			{pre=nxt=nullptr;}
	};
	
};

#endif
