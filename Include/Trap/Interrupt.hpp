#ifndef POS_INTERRUPT_HPP
#define POS_INTERRUPT_HPP

#include "../Riscv.h"

inline void InterruptEnable()
{
	set_csr(sstatus,SSTATUS_SIE);
}

inline void InterruptDisable()
{
	clear_csr(sstatus,SSTATUS_SIE);
}

class InterruptStackSaver
{
	protected:
		bool x=0;
		
	public:
		inline void Save()
		{
			if (read_csr(sstatus)&SSTATUS_SIE)
			{
				InterruptDisable();
				x=true;
			}
			else x=false;
		}
		
		inline void Restore()
		{
			if (x)
			{
				InterruptEnable();
				x=false;
			}
		}
		
		inline bool operator * () const
		{return x;}
};

class InterruptStackAutoSaver:public InterruptStackSaver
{
	public:
		inline ~InterruptStackAutoSaver()
		{
			Restore();
		}
		
		inline InterruptStackAutoSaver()//Correct usage: InterruptStackAutoSaver isas; //Not isas() ! (?)
		{
			Save();
		}
};

class InterruptStackAutoSaverBlockController:public InterruptStackSaver
{
	public:
		bool f;
		
		inline ~InterruptStackAutoSaverBlockController()
		{
			Restore();
		}
		
		inline InterruptStackAutoSaverBlockController():f(1)
		{
			Save();
		}
};

#define ISAS for (InterruptStackAutoSaverBlockController isasbc;isasbc.f;isasbc.f=0)//??

#endif
