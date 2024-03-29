#include <Memory/VirtualMemory.hpp>
#include <Process/Process.hpp>
#include <Error.hpp>
#include <Memory/PhysicalMemory.hpp>
#include <Library/TemplateTools.hpp>
#include <Library/Kout.hpp>

VirtualMemorySpace *VirtualMemorySpace::CurrentVMS=nullptr,
				   *VirtualMemorySpace::BootVMS=nullptr,
				   *VirtualMemorySpace::KernelVMS=nullptr;

using namespace POS;

ErrorType PageTable::Destroy(const int level)
{
	ASSERT(level>=0,"PageTable::Destroy level<0!");
//	kout[Test]<<"PageTable::Destroy "<<this<<" with level "<<level<<endl;
	for (int i=0;i<PageTableEntryCount;++i)
		if (entries[i].Valid())
			if (entries[i].IsPageTable())
				entries[i].GetPageTable()->Destroy(level-1);
			else if (entries[i].Get<PageTable::Entry::U>()==0)
				DoNothing;//Kernel page won't be freed
			else if (level==0)//??
			{
				Page *page=entries[i].GetPage();
				ASSERT(page,"PageTable::Destroy: page is nullptr!");
				if (--page->ref==0)
				{
					POS_PMM.FreePage(page);
				}
			}
			else kout[Warning]<<"PageTable::Destroy free huge page is not usable!"<<endl;
	POS_PMM.FreePage(POS_PMM.GetPageFromAddr(this));
//	kout[Test]<<"PageTable::Destroy "<<this<<" with level "<<level<<" OK"<<endl;
	return ERR_Todo;
}

ErrorType VirtualMemoryRegion::CopyMemory(PageTable &pt,const PageTable &src,int level,Uint64 l)
{
	ASSERTEX(level>=0,"VirtualMemoryRegion::CopyMemory of "<<this<<" from "<<src<<" level "<<level<<" <0!");
	if (Flags&VM_Kernel)
	{
//		kout[Warning]<<"VirtualMemoryRegion::CopyMemory kernel region don't need copy?"<<endl;
		return ERR_None;
	}
	for (int i=0;i<PageTable::PageTableEntryCount;++i,l+=PageSizeN[level])
		if (src[i].Valid()&&Intersect(l,l+PageSizeN[level]))
			if (src[i].IsPageTable())
			{
				PageTable *srcNxt=src[i].GetPageTable(),
						  *ptNxt=nullptr;
				if (!pt[i].Valid())
				{
					ptNxt=(PageTable*)POS_PMM.AllocPage(1)->KAddr();
					ASSERTEX(ptNxt,"VirtualMemoryRegion::CopyMemory: Cannot allocate ptNxt in VMR "<<this);
					ASSERTEX(((PtrInt)ptNxt&(PageSize-1))==0,"ptNxt "<<ptNxt<<" in "<<this<<" is not aligned to 4k!");
					ptNxt->Init();
					pt[i].SetPageTable(ptNxt);
				}
				else ptNxt=pt[i].GetPageTable();
				CopyMemory(*ptNxt,*srcNxt,level-1,l);
			}
			else if (level==0)
			{
				Page *srcPage=src[i].GetPage();
				if (Flags&VM_Shared|Flags&VM_Kernel)
					pt[i].SetPage(srcPage,ToPageEntryFlags());
				else
				{
					Page *page=POS_PMM.AllocPage(1);
					ASSERTEX(page,"VirtualMemoryRegion::CopyMemory: Cannot allocate page in VMR "<<this);
					ASSERTEX(((PtrInt)page->PAddr()&(PageSize-1))==0,"page->Paddr() "<<page->PAddr()<<" is not aligned to 4k!");
					ASSERTEX(!pt[i].Valid(),"VirtualMemoryRegion::CopyMemory: pt[i] is valid!");
					MemcpyT<char>((char*)page->KAddr(),(const char*)srcPage->KAddr(),PageSize);
					pt[i].SetPage(page,ToPageEntryFlags());
				}
			}
			else kout[Warning]<<"VirtualMemoryRegion::CopyMemory copy huge page is not usable!"<<endl;
	return ERR_None;
}

ErrorType VirtualMemoryRegion::Init(PtrInt start,PtrInt end,Uint32 flags)
{
	LinkTableT::Init();
	VMS=nullptr;
	Start=start>>PageSizeBit<<PageSizeBit;
	End=end+PageSize-1>>PageSizeBit<<PageSizeBit;//??
	Flags=flags;
	ASSERTEX(Start<End,"VirtualMemoryRegion::Init: Start "<<(void*)Start<<" >= End "<<(void*)End);
	return ERR_None;
}

ErrorType VirtualMemorySpace::ClearVMR()
{
//	kout[Test]<<"VirtualMemorySpace::ClearVMR "<<this<<endl;
	while (VmrHead.Nxt())
		RemoveVMR(VmrHead.Nxt(),1);
	return ERR_None;
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
	ASSERT(vmr->VMS==nullptr,"VirtualMemorySpace::InsertVMR: vmr->VMS is not nullptr!");
	vmr->VMS=this;
	if (VmrHead.Nxt()==nullptr)
		VmrHead.NxtInsert(vmr);
	else if (VmrCache!=nullptr&&VmrCache->Nxt()==nullptr&&VmrCache->End<=vmr->Start)
		VmrCache->NxtInsert(vmr);
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

void VirtualMemorySpace::RemoveVMR(VirtualMemoryRegion *vmr,bool FreeVmr)
{
	ASSERT(vmr,"VirtualMemorySpace::RemoveVMR: vmr is nullptr!");
	if (VmrCache==vmr)
		VmrCache=nullptr;
	//<<Invalidate vmr's Page valid bit...
	vmr->Remove();
	vmr->VMS=nullptr;
	--VmrCount;
	if (FreeVmr)
		Kfree(vmr);
}

PtrInt VirtualMemorySpace::GetUsableVMR(PtrInt start,PtrInt end,PtrInt length)//return 0 means invalid
{
	if (start>=end||length==0||end-start<length)
		return 0;
	start=start>>PageSizeBit<<PageSizeBit;
	end=end+PageSize-1>>PageSizeBit<<PageSizeBit;
	length=length+PageSize-1>>PageSizeBit<<PageSizeBit;
	if (VmrHead.Nxt()==nullptr||maxN(PageSize*4ull,start)+length<=minN(VmrHead.Nxt()->Start,end))
		return maxN(PageSize*4ull,start);
	for (VirtualMemoryRegion *vmr=VmrHead.Nxt();vmr;vmr=vmr->Nxt())
		if (vmr->Nxt()==nullptr)//??
			return 0;
		else if (maxN(vmr->End,start)+length<=minN(vmr->Nxt()->Start,end))
			return maxN(vmr->End,start);
	return 0;
}

//VirtualMemoryRegion* VirtualMemorySpace::CopyVMR()
//{
////	VirtualMemoryRegion *vmrs=(VirtualMemoryRegion*)Kmalloc(sizeof(VirtualMemoryRegion)*);
//	return nullptr;
//}

ErrorType VirtualMemorySpace::InitForBoot()
{
	kout[Info]<<"Initing Boot VirtualMemorySpace..."<<endl;
	BootVMS=KmallocT<VirtualMemorySpace>();
	BootVMS->Init();
	{
		auto vmr=KmallocT<VirtualMemoryRegion>();
		vmr->Init((PtrInt)kernelstart,PhysicalMemoryVirtualEnd(),VirtualMemoryRegion::VM_KERNEL);
		BootVMS->InsertVMR(vmr);
	}
//	{
//		auto vmr=KmallocT<VirtualMemoryRegion>();
//		vmr->Init(10000,20000,VirtualMemoryRegion::VM_TEST);//Test region for pagefault...
//		BootVMS->InsertVMR(vmr);
//	}
	{
		auto vmr=KmallocT<VirtualMemoryRegion>();
		vmr->Init(PhymemVirmemOffset(),PhymemVirmemOffset()+0x80000000,VirtualMemoryRegion::VM_MMIO);
		BootVMS->InsertVMR(vmr);
	}
	BootVMS->PDT=PageTable::Boot();
	BootVMS->SharedCount=1;
	kout[Info]<<"Init BootVMS "<<BootVMS<<" OK"<<endl;
	return ERR_None;
}

ErrorType VirtualMemorySpace::InitForKernel()
{
	KernelVMS=BootVMS;
	kout[Info]<<"Init KernelVMS "<<KernelVMS<<" OK"<<endl;
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
	--SharedCount;
}

void VirtualMemorySpace::Ref(Process *proc)
{
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
//	kout[Test]<<"VirtualMemorySpace::Enter: "<<this<<endl;
	CurrentVMS=this;
	lcr3((Uint64/*??!*/)PDT->PAddr());//??
	asm volatile("sfence.vma \n fence.i \n fence");
}

ErrorType VirtualMemorySpace::CreatePDT()
{
	PDT=(PageTable*)POS_PMM.AllocPage(1)->KAddr();
	ASSERTEX(((PtrInt)PDT&(PageSize-1))==0,"PDT "<<PDT<<" in VMS "<<this<<" is not aligned to 4k!");
	PDT->InitAsPDT();
	return ERR_None;
}

ErrorType VirtualMemorySpace::Create(int type)
{
//	kout[Test]<<"VirtualMemorySpace::Create "<<this<<" as type "<<type<<endl;
	CreatePDT();
	{
		auto vmr=KmallocT<VirtualMemoryRegion>();
		vmr->Init((PtrInt)kernelstart,PhysicalMemoryVirtualEnd(),VirtualMemoryRegion::VM_KERNEL);
		InsertVMR(vmr);
	}
	{
		auto vmr=KmallocT<VirtualMemoryRegion>();
		vmr->Init(PhymemVirmemOffset(),PhymemVirmemOffset()+0x80000000,VirtualMemoryRegion::VM_MMIO);
		InsertVMR(vmr);
	}
	switch (type)
	{
		case VMS_Default:	break;
		case VMS_CurrentTest:
			kout[Warning]<<"VirtualMemorySpace::Create CurrentTest type VMS!"<<endl;
			break;
		default:
			kout[Fault]<<"VirtualMemorySpace::Create unknown type VMS "<<type<<endl;
	}
//	kout[Test]<<"VirtualMemorySpace::Create "<<this<<" OK"<<endl;
	return ERR_None;
}

ErrorType VirtualMemorySpace::CreateFrom(VirtualMemorySpace *vms)
{
	kout[Test]<<"VirtualMemorySpace::CreateFrom "<<vms<<", this is "<<this<<endl;
	CreatePDT();
	VirtualMemoryRegion *p=vms->VmrHead.Nxt(),*q=nullptr;
	while (p)
	{
		q=KmallocT<VirtualMemoryRegion>();
		ASSERTEX(q,"VirtualMemorySpace::CreateFrom failed to allocate VMR!");
		q->Init(p->Start,p->End,p->Flags);
		InsertVMR(q);
		q->CopyMemory(*PDT,*vms->PDT,2);
		p=p->Nxt();
	}
	kout[Test]<<"VirtualMemorySpace::CreateFrom "<<vms<<", OK this is "<<this<<endl;
	return ERR_Todo;
}

ErrorType VirtualMemorySpace::SolvePageFault(TrapFrame *tf)
{
//	kout[Test]<<"SolvePageFault in "<<(void*)tf->badvaddr<<endl;
	VirtualMemoryRegion* vmr=FindVMR(tf->badvaddr);
	if (vmr==nullptr)
		return ERR_AccessInvalidVMR;
	{//Need improve...
		PageTable::Entry &e2=(*PDT)[PageTable::VPN<2>(tf->badvaddr)];
		PageTable *pt2;
		if (!e2.Valid())
		{
			Page *pg2=POS_PMM.AllocPage(1);
			if (pg2==nullptr)
				return ERR_OutOfMemory;
			pt2=(PageTable*)pg2->KAddr();
			ASSERT(((PtrInt)pt2&(PageSize-1))==0,"pt2 is not aligned to 4k!");
			pt2->Init();
			e2.SetPageTable(pt2);
		}
		else pt2=e2.GetPageTable();
		
		PageTable::Entry &e1=(*pt2)[PageTable::VPN<1>(tf->badvaddr)];
		PageTable *pt1;
		if (!e1.Valid())
		{
			Page *pg1=POS_PMM.AllocPage(1);
			if (pg1==nullptr)
				return ERR_OutOfMemory;
			pt1=(PageTable*)pg1->KAddr();
			ASSERT(((PtrInt)pt1&(PageSize-1))==0,"pt1 is not aligned to 4k!");
			pt1->Init();
			e1.SetPageTable(pt1);
		}
		else pt1=e1.GetPageTable();
		
		PageTable::Entry &e0=(*pt1)[PageTable::VPN<0>(tf->badvaddr)];
		Page *pg0;
		if (!e0.Valid())
		{
			pg0=POS_PMM.AllocPage(1);
			if (pg0==nullptr)
				return ERR_OutOfMemory;
			ASSERT(((PtrInt)pg0->PAddr()&(PageSize-1))==0,"pg0->Paddr() is not aligned to 4k!");
			MemsetT<char>((char*)pg0->KAddr(),0,PageSize);//Clear the page data that no data will leak and also for pointer safty.
			e0.SetPage(pg0,vmr->ToPageEntryFlags());
		}
		else kout[Fault]<<"VirtualMemorySpace::SolvePageFault: Page exist, however page fault!"<<endl;
		asm volatile("sfence.vma \n fence.i \n fence");//Test usage... Need improve!
//		asm volatile("sfence.vm");
	}
//	kout[Test]<<"SolvePageFault OK"<<endl;
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
//	kout[Test]<<"VirtualMemorySpace::Destroy "<<this<<endl;
	ASSERT(this!=Current(),"VirtualMemorySpace::Destroy this is current!");
	if (SharedCount!=0)
		kout[Fault]<<"VirtualMemorySpace::Destroy "<<this<<" while SharedCount "<<SharedCount<<" is not zero!"<<endl;
	ClearVMR();
	if (PDT!=nullptr)
		PDT->Destroy(2);//??
//	kout[Test]<<"VirtualMemorySpace::Destroy "<<this<<" OK"<<endl;
	return ERR_None;
}

ErrorType TrapFunc_FageFault(TrapFrame *tf)
{
	return VirtualMemorySpace::Current()->SolvePageFault(tf);//??
}
