#ifndef POS_VIRTUALMEMORY_HPP
#define POS_VIRTUALMEMORY_HPP

#include "../Types.hpp"
#include "../Library/DataStructure/LinkTable.hpp"
#include "../Library/TemplateTools.hpp"
#include "PhysicalMemory.hpp"
#include "../Trap/Trap.hpp"
#include "../Library/Kout.hpp"
#include "../Riscv.h"
#include "../Process/SpinLock.hpp"
#include "../Config.h"

class PageTable;

extern "C"
{
	extern PageTable boot_page_table_sv39[];
};

class PageTable
{
	public:
		struct Entry
		{
			enum:unsigned {IndexBits=6};
			
			enum:unsigned
			{
				//Format: Length|Index
				V=1<<IndexBits|0,
				R=1<<IndexBits|1,
				W=1<<IndexBits|2,
				X=1<<IndexBits|3,
				U=1<<IndexBits|4,
				G=1<<IndexBits|5,
				A=1<<IndexBits|6,
				D=1<<IndexBits|7,
				
				XWR=3<<IndexBits|1,
				GU=2<<IndexBits|4,
				GUXWR=5<<IndexBits|1,
				
				RSW=2<<IndexBits|8,
				RSW0=1<<IndexBits|8,
				RSW1=1<<IndexBits|9,
				
				PPN=44<<IndexBits|10,
				PPN0=9<<IndexBits|10,
				PPN1=9<<IndexBits|19,
				PPN2=26<<IndexBits|28,
//				PPNS=1<<IndexBits|53,
				PPN_RESERVED=10<<IndexBits|54,
				
				FLAGS=8<<IndexBits|0,
				All=64<<IndexBits|0
			};
			
			enum
			{
				XWR_PageTable=0,
				XWR_ReadOnly=1,
				XWR_ReadWrite=3,
				XWR_ExecOnly=4,
				XWR_ReadExec=5,
				XWR_ReadWriteExec=7
			};
			
			template <unsigned B> inline static constexpr PageTableEntryType Mask()
			{
				constexpr PageTableEntryType i=B&((1<<IndexBits)-1),
								   			 m=((PageTableEntryType)1<<(B>>IndexBits))-1;
				return m<<i;
			}
			
			PageTableEntryType x;
			
			inline PageTableEntryType& operator () () {return x;}
			inline PageTableEntryType& operator * () {return x;}
			
			template <unsigned B> inline PageTableEntryType Get() const
			{
				constexpr PageTableEntryType i=B&((1<<IndexBits)-1),
								   			 m=((PageTableEntryType)1<<(B>>IndexBits))-1;
				return (x>>i)&m;
			}
			
			template <unsigned B> inline void Set(PageTableEntryType y)
			{
				constexpr PageTableEntryType i=B&((1<<IndexBits)-1),
											 m=~(((PageTableEntryType)1<<(B>>IndexBits))-1<<i);//??
				x=(x&m)|y<<i;
			}
			
			template <unsigned I,unsigned L> inline PageTableEntryType Get() const
			{return Get< L<<IndexBits|I >();}
			
			template <unsigned I,unsigned L> inline void Set(PageTableEntryType y)
			{return Set< L<<IndexBits|I >();}
			
			inline bool Valid() const
			{return Get<V>();}
			
			inline void* GetPPN_KAddr() const
			{
				return (void*)((Get<PPN>()<<PageSizeBit)+PhymemVirmemOffset());//??
			}
			
			inline bool IsPageTable() const
			{return Get<XWR>()==XWR_PageTable;}
			
			inline PageTable* GetPageTable() const//Won't check! need use IsSubPageTable before
			{return (PageTable*)GetPPN_KAddr();}
			
			inline void SetPageTable(PageTable *pt)
			{
				Set<V>(1);
				Set<XWR>(XWR_PageTable);
				Set<PPN>((PageTableEntryType)pt->PAddr()>>PageSizeBit);
			}
			
			inline Page* GetPage() const
			{return POS_PMM.GetPageFromAddr(GetPPN_KAddr());}
			
			inline void SetPage(Page *pg,PageTableEntryType flags)
			{
				++pg->ref;
				Set<FLAGS>(flags);
				Set<PPN>((PageTableEntryType)pg->PAddr()>>PageSizeBit);
			}
			
			inline Entry(PageTableEntryType _x):x(_x) {}
			inline Entry() {}
		};
		
		enum
		{
			PageTableEntryCountBit=9,
			PageTableEntryCount=1<<PageTableEntryCountBit
		};
		
	protected:
		Entry entries[PageTableEntryCount];
		
	public:
		template <unsigned n> static inline unsigned VPN(PtrInt kaddr)
		{
			constexpr unsigned i=PageSizeBit+n*PageTableEntryCountBit,
							   m=(1<<PageTableEntryCountBit)-1;
			return kaddr>>i&m;
		}
		
		static inline unsigned VPN(PtrInt kaddr,unsigned n)
		{
			unsigned i=PageSizeBit+n*PageTableEntryCountBit,
					 m=(1<<PageTableEntryCountBit)-1;
			return kaddr>>i&m;
		}
		
		inline static PageTable* Boot()
		{return boot_page_table_sv39;}
		
		inline void* KAddr()
		{return this;}
		
		inline void* PAddr()
		{return (void*)((Uint64)this-PhymemVirmemOffset());}

		inline Entry& operator [] (int i)
		{return entries[i];}
		
		inline const Entry& operator [] (int i) const
		{return entries[i];}
		
		inline ErrorType InitAsPDT()
		{
			using namespace POS;
//			kout[Test]<<"PageTable::InitAsPDT "<<this<<endl;
			POS::MemsetT(entries,Entry(0),PageTableEntryCount-3);
			POS::MemcpyT(&entries[PageTableEntryCount-3],&(*Boot())[PageTableEntryCount-3],3);//??
			return ERR_None;
		}
		
		inline ErrorType Init()
		{
			using namespace POS;
//			kout[Test]<<"PageTable::Init "<<this<<endl;
			POS::MemsetT(entries,Entry(0),PageTableEntryCount);
			return ERR_None;
		}
		
		inline ErrorType Destroy(const int level);
};

class VirtualMemorySpace;
class VirtualMemoryRegion:public POS::LinkTableT <VirtualMemoryRegion>
{
	friend class VirtualMemorySpace;
	friend void KernelFaultSolver();
	public:
		enum:Uint32
		{
			VM_Read		=1<<0,
			VM_Write	=1<<1,
			VM_Exec		=1<<2,
			VM_Stack	=1<<3,
			VM_Heap		=1<<4,
			VM_Kernel	=1<<5,
			VM_Shared	=1<<6,
			VM_Device	=1<<7,
			VM_File		=1<<8,
			VM_Dynamic	=1<<9,
			
			VM_RW=VM_Read|VM_Write,
			VM_RWX=VM_RW|VM_Exec,
			VM_KERNEL=VM_Kernel|VM_RWX,
			VM_USERSTACK=VM_RW|VM_Stack|VM_Dynamic,
			VM_USERHEAP=VM_RW|VM_Heap|VM_Dynamic,
			VM_MMIO=VM_RW|VM_Kernel|VM_Device,
			
			VM_TEST=VM_Kernel|VM_RW|VM_Shared//??
		};
	
	protected:
		VirtualMemorySpace *VMS;
		PtrInt Start,//should align to page
			   End;
		Uint32 Flags;
		
		ErrorType CopyMemory(PageTable &pt,const PageTable &src,int level,Uint64 l=0);//Only copy valid area of this and target.
		
	public:
		inline PageTableEntryType ToPageEntryFlags()
		{
			using PageEntry=PageTable::Entry;
			PageTableEntryType re=PageTable::Entry::Mask<PageTable::Entry::V>();
			if (Flags&VM_Read)
				re|=PageTable::Entry::Mask<PageTable::Entry::R>();
			if (Flags&VM_Write)
				re|=PageTable::Entry::Mask<PageTable::Entry::W>();
			if (Flags&VM_Exec)
				re|=PageTable::Entry::Mask<PageTable::Entry::X>();
			if (Flags&VM_Kernel)
				re|=PageTable::Entry::Mask<PageTable::Entry::G>();
			else re|=PageTable::Entry::Mask<PageTable::Entry::U>();
			return re;
		}
		
		inline void Memset0()
		{POS::MemsetT<char>((char*)Start,0,End-Start);}
		
		inline bool Intersect(PtrInt l,PtrInt r) const
		{return r>Start&&End>l;}
		
		inline bool In(PtrInt l,PtrInt r) const//[l,r)
		{return Start<=l&&r<=End;}
		
		inline bool In(PtrInt p)
		{return Start<=p&&p<End;}
		
		inline PtrInt GetStart()
		{return Start;}
		
		inline PtrInt GetEnd()
		{return End;}
		
		inline PtrInt GetLength()
		{return End-Start;}
		
		ErrorType Init(PtrInt start,PtrInt end,Uint32 flags);
};

class Process;

class VirtualMemorySpace:protected SpinLock
{
	friend void KernelFaultSolver();
	public:
		enum
		{
			VMS_Default=0,
			VMS_CurrentTest,
			VMS_InnerUserProcess
		};
	
	protected:
		static VirtualMemorySpace *CurrentVMS,
								  *BootVMS,
								  *KernelVMS;
		
		POS::LinkTableT <VirtualMemoryRegion> VmrHead;
		Uint32 VmrCount;
		VirtualMemoryRegion *VmrCache;//Recent found VMR
		PageTable *PDT;
		Uint32 SharedCount;
		
//		VirtualMemoryRegion* CopyVMR();
		ErrorType ClearVMR();
		ErrorType CreatePDT();
		
		static ErrorType InitForBoot();
		static ErrorType InitForKernel();
		
	public:
		inline static VirtualMemorySpace* Current()
		{return CurrentVMS;}
		
		inline static VirtualMemorySpace* Boot()
		{return BootVMS;}
		
		inline static VirtualMemorySpace* Kernel()
		{return KernelVMS;}
		
		inline static void EnableAccessUser()
		{
			#ifdef QEMU
			write_csr(sstatus,read_csr(sstatus)|SSTATUS_SUM);
			#else
			write_csr(sstatus,read_csr(sstatus)&~SSTATUS_SUM);
			#endif
		}
		
		inline static void DisableAccessUser()
		{
			#ifdef QEMU
			write_csr(sstatus,read_csr(sstatus)&~SSTATUS_SUM);
			#else
			write_csr(sstatus,read_csr(sstatus)|SSTATUS_SUM);
			#endif
		}
		
		static ErrorType InitStatic();
		
		VirtualMemoryRegion* FindVMR(PtrInt p);
		void InsertVMR(VirtualMemoryRegion *vmr);
		void RemoveVMR(VirtualMemoryRegion *vmr,bool FreeVmr);
		PtrInt GetUsableVMR(PtrInt start,PtrInt end,PtrInt length);
		
		void Unref(Process *proc);
		void Ref(Process *proc);
		bool TryDeleteSelf();//Should not be kernel common space!
		void Enter();
		
		inline void Leave()
		{
			if (CurrentVMS==this)
				KernelVMS->Enter();
		}
		
		ErrorType SolvePageFault(TrapFrame *tf);
		ErrorType Create(int type=VMS_Default);
		ErrorType CreateFrom(VirtualMemorySpace *vms);//Remember call destroy and init first if not new.
		
		ErrorType Init();
		ErrorType Destroy();
};

class HeapMemoryRegion:public VirtualMemoryRegion
{
	protected:
		Uint64 BreakPointLength=0;
		
	public:
		inline PtrInt BreakPoint()
		{return Start+BreakPointLength;}
		
		inline ErrorType Resize(Sint64 delta)
		{
			if (delta>=0)
			{
				BreakPointLength+=delta;
				if (Start+BreakPointLength>End)
					if (nxt==nullptr||Start+BreakPointLength<=nxt->GetStart())
						End=Start+BreakPointLength+PageSize-1>>PageSizeBit<<PageSizeBit;
					else return ERR_HeapCollision;
			}
			else
			{
				if (-delta>BreakPointLength)
				{
					BreakPointLength=0;
					End=Start+PageSize;
				}
				else
				{
					BreakPointLength+=delta;
					End=Start+BreakPointLength+PageSize-1>>PageSizeBit<<PageSizeBit;
				}
				//<<destroy uneeded pages...
			}
			return ERR_None;
		}
		
		inline ErrorType Init(PtrInt start,Uint64 len=PageSize,Uint64 flags=VM_USERHEAP)
		{
			BreakPointLength=len;
			return VirtualMemoryRegion::Init(start,start+len,flags);
		}
};

ErrorType TrapFunc_FageFault(TrapFrame *tf);

#endif
