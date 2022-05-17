#include <Memory/VirtualMemory.hpp>
#include <Process/Process.hpp>
#include <Error.hpp>
#include <Memory/PhysicalMemory.hpp>
#include <Library/TemplateTools.hpp>
#include <Library/Kout.hpp>

#include "../../Include/Memory/VirtualMemory.hpp"
#include "../../Include/Process/Process.hpp"
#include "../../Include/Trap/Trap.hpp"

VirtualMemorySpace *VirtualMemorySpace::CurrentVMS=nullptr,
				   *VirtualMemorySpace::BootVMS=nullptr,
				   *VirtualMemorySpace::KernelVMS=nullptr;

using namespace POS;

ErrorType PageTable::Destroy()
{
	kout[Test]<<"PageTable::Destroy "<<this<<endl;
	for (int i=0;i<PageTableEntryCount;++i)
		if (entries[i].Valid())
			if (entries[i].IsPageTable())
				entries[i].GetPageTable()->Destroy();
			else
			{
				Page *page=entries[i].GetPage();
				ASSERT(page,"PageTable::Destroy: page is nullptr!");
				if (--page->ref==0)
				{
					kout[Test]<<"Free page "<<page<<" addr "<<(void*)(entries[i].Get<Entry::PPN>()<<PageSizeBit)<<endl;
					POS_PMM.FreePage(page);
				}
			}
	kout[Test]<<"PageTable::Destroy "<<this<<" OK"<<endl;
	Kfree(this);
	return ERR_Todo;
}

ErrorType VirtualMemoryRegion::Init(PtrInt start,PtrInt end,PtrInt flags)
{
	LinkTableT::Init();
	Start=start>>PageSizeBit<<PageSizeBit;
	End=end+PageSize-1>>PageSizeBit<<PageSizeBit;//??
	Flags=flags;
	ASSERT(Start<End,"VirtualMemoryRegion::Init: Start>=End!");
	return ERR_None;
}

ErrorType VirtualMemoryRegion::Destroy()
{
	kout[Warning]<<"VirtualMemoryRegion::Destroy is not usable!"<<endl;
	return ERR_Todo;
}

VirtualMemoryRegion* VirtualMemorySpace::FindVMR(PtrInt p)
{
	if (VmrCache!=nullptr&&VmrCache->In(p))
		return VmrCache;
	VirtualMemoryRegion *v=VmrHead.Nxt();
	while (v!=nullptr)
		if (p<v->Start)
			return nullptr;
		else if (p<v->End)
			return VmrCache=v;
		else v=v->Nxt();
	return nullptr;
}

void VirtualMemorySpace::InsertVMR(VirtualMemoryRegion *vmr)
{
	ASSERT(vmr,"VirtualMemorySpace::InsertVMR: vmr is nullptr!");
	ASSERT(vmr->Single(),"VirtualMemorySpace::InsertVMR: vmr is not single!");
	if (VmrHead.Nxt()==nullptr)
		VmrHead.NxtInsert(vmr);
	else
	{
		VirtualMemoryRegion *v=VmrHead.Nxt();
		VMS_INSERTVMR_WHILE:
			if (vmr->End<=v->Start)
				v->PreInsert(vmr);
			else if (vmr->Start>=v->End)
				if (v->Nxt()==nullptr)
					v->NxtInsert(vmr);
				else
				{
					v=v->Nxt();
					if (v!=nullptr)
						goto VMS_INSERTVMR_WHILE;
				}
			else kout[Fault]<<"VirtualMemoryRegion::InsertVMR: vmr "<<vmr<<" overlapped with "<<v<<endl;
	}
	VmrCache=vmr;
	++VmrCount;
}

void VirtualMemorySpace::RemoveVMR(VirtualMemoryRegion *vmr)
{
	ASSERT(vmr,"VirtualMemorySpace::RemoveVMR: vmr is nullptr!");
	if (VmrCache==vmr)
		VmrCache=nullptr;
	vmr->Remove();
	--VmrCount;
}

VirtualMemoryRegion* VirtualMemorySpace::CopyVMR()
{
//	VirtualMemoryRegion *vmrs=(VirtualMemoryRegion*)Kmalloc(sizeof(VirtualMemoryRegion)*);
	return nullptr;
}

PageTable* VirtualMemorySpace::CopyPDT()
{
	
	return nullptr;
}

ErrorType VirtualMemorySpace::InitForBoot()
{
	BootVMS=KmallocT<VirtualMemorySpace>();
	BootVMS->Init();
	auto vmr=KmallocT<VirtualMemoryRegion>();
	vmr->Init((PtrInt)kernelstart,PhysicalMemoryVirtualEnd(),VirtualMemoryRegion::VM_KERNEL);
	BootVMS->InsertVMR(vmr);
	{
		vmr=KmallocT<VirtualMemoryRegion>();
		vmr->Init(10000,20000,VirtualMemoryRegion::VM_TEST);//Test region for pagefault...
		BootVMS->InsertVMR(vmr);
	}
	BootVMS->PDT=PageTable::Boot();
	BootVMS->SharedCount=1;
	return ERR_None;
}

ErrorType VirtualMemorySpace::InitForKernel()
{
	KernelVMS=BootVMS;
	return ERR_None;
}

ErrorType VirtualMemorySpace::InitStatic()
{
	InitForBoot();
	InitForKernel();
	CurrentVMS=KernelVMS;
	return ERR_None;
}
		
void VirtualMemorySpace::Unref(Process *proc)
{
	kout[Warning]<<"VirtualMemorySpace::Unref is uncompleted!"<<endl;
	--SharedCount;
}

void VirtualMemorySpace::Ref(Process *proc)
{
	kout[Warning]<<"VirtualMemorySpace::Ref is uncompleted!"<<endl;
	++SharedCount;
}

bool VirtualMemorySpace::TryDeleteSelf()
{
	if (SharedCount==0&&POS::NotInSet(this,BootVMS,KernelVMS))
	{
		if (this==CurrentVMS)
			Leave();
		Destroy();
		Kfree(this);
		return 1;
	}
	else return 0;
}

void VirtualMemorySpace::Enter()
{
	if (this==CurrentVMS)
		return;
	kout[Test]<<"VirtualMemorySpace::Enter: "<<PDT<<endl;
	lcr3((Uint64/*??!*/)PDT->PAddr());//??
}

ErrorType VirtualMemorySpace::Create()
{
	kout[Warning]<<"VirtualMemorySpace::Create is not usable!"<<endl;
	return ERR_Todo;
}

ErrorType VirtualMemorySpace::CopyFrom(VirtualMemorySpace *vms)
{
	kout[Warning]<<"VirtualMemorySpace::CopyFrom if not usable!"<<endl;
	return ERR_Todo;
}

ErrorType VirtualMemorySpace::SolvePageFault(TrapFrame *tf)
{
	kout[Test]<<"SolvePageFault in "<<tf->badvaddr<<endl;
	VirtualMemoryRegion* vmr=FindVMR(tf->badvaddr);
	if (vmr==nullptr)
		return ERR_AccessInvalidVMR;
	{//Need improve...
		PageTable::Entry &e2=(*PDT)[PageTable::VPN<2>(tf->badvaddr)];
		kout[Debug]<<"VPN2 "<<PageTable::VPN<2>(tf->badvaddr)<<endl;
		PageTable *pt2;
		if (!e2.Valid())
		{
			pt2=(PageTable*)POS_PMM.AllocPage(1)->KAddr();
			ASSERT(pt2,"VirtualMemorySpace::SolvePageFault: Cannot Kmalloc pt2!");
			pt2->Init();
			e2.SetPageTable(pt2);
		}
		else pt2=e2.GetPageTable();
		
		PageTable::Entry &e1=(*pt2)[PageTable::VPN<1>(tf->badvaddr)];
		kout[Debug]<<"VPN1 "<<PageTable::VPN<1>(tf->badvaddr)<<endl;
		PageTable *pt1;
		if (!e1.Valid())
		{
			pt1=(PageTable*)POS_PMM.AllocPage(1)->KAddr();
			ASSERT(pt1,"VirtualMemorySpace::SolvePageFault: Cannot Kmalloc pt1!");
			pt1->Init();
			e1.SetPageTable(pt1);
		}
		else pt1=e1.GetPageTable();
		
		PageTable::Entry &e0=(*pt1)[PageTable::VPN<0>(tf->badvaddr)];
		kout[Debug]<<"VPN0 "<<PageTable::VPN<0>(tf->badvaddr)<<endl;
		Page *pg0;
		if (!e0.Valid())
		{
			pg0=POS_PMM.AllocPage(1);
			ASSERT(pg0,"VirtualMemorySpace::SolvePageFault: Cannot allocate Page pg0!");
			MemsetT<char>((char*)pg0->KAddr(),0,PageSize);//Needed??
			e0.SetPage(pg0,vmr->ToPageEntryFlags());
		}
		else kout[Fault]<<"VirtualMemorySpace::SolvePageFault: Page exist, however page fault!"<<endl;
		asm volatile("sfence.vma");//Test usage... Need improve!
	}
	kout[Test]<<"SolvePageFault OK"<<endl;
	return ERR_None;
}

ErrorType VirtualMemorySpace::Init()
{
	VmrHead.Init();
	VmrCount=0;
	VmrCache=nullptr;
	PDT=nullptr;
	SharedCount=0;
	SpinLock::Init();
	return ERR_None;
}

ErrorType VirtualMemorySpace::Destroy()
{
	kout[Warning]<<"VirtualMemorySpace::Destroy is not usable!"<<endl;
	return ERR_Todo;
}

ErrorType TrapFunc_FageFault(TrapFrame *tf)
{
	return POS_PM.Current()->GetVMS()->SolvePageFault(tf);//??
}
