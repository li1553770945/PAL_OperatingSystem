#include <Library/Kout.hpp>

namespace POS
{
	KOUT kout;
};

#include <Process/Process.hpp>

void CallingStackController::PrintCallingStack(char **p)
{
	using namespace POS;
	InterruptStackAutoSaver isas;
	kout<<LightYellow<<"CallingStack:"<<endl;
	while (p)
	{
		--p;
		kout<<LightYellow<<*p<<endl;
		if ((PtrInt)(p-1)%PageSize==0)
			if (*--p==nullptr)
				p=nullptr;
			else p=(char**)*p;
	}
}

CallingStackController::~CallingStackController()
{
	using namespace POS;
	InterruptStackAutoSaver isas;
	--p;
	if (*p!=name)
		kout[Fault]<<"CallingStackController: Calling unpaired in "<<*p<<" | "<<name<<endl;
	if ((PtrInt)(p-1)%PageSize==0)
		if (*--p==nullptr)
		{
			delete[] p;
			p=nullptr;
		}
		else
		{
			char **q=(char**)*p;
			delete[] p;
			p=q;
		}
}

CallingStackController::CallingStackController(const char *_name):name(_name),p(POS_PM.Current()->CallingStack)
{
	using namespace POS;
	InterruptStackAutoSaver isas;
	ASSERTEX(name,"CallingStackController: name is nullptr");
	if (p==nullptr)
	{
		p=new char*[PageSize];
		MemsetT<char>((char*)p,0,PageSize);
		p++;
	}
	else if ((PtrInt)p%PageSize==0)
	{
		char **q=p;
		p=new char*[PageSize];
		MemsetT<char>((char*)p,0,PageSize);
		*p=(char*)q;
		++p;
	}
	*p=(char*)name;
	++p;
}
