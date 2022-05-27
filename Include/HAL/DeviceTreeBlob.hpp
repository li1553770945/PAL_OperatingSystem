#ifndef POS_DEVICETREEBLOB_HPP
#define POS_DEVICETREEBLOB_HPP

#include "../Types.hpp"
#include "../Library/TemplateTools.hpp"
#include "../Library/Kout.hpp"

struct DTB_BootParamHeader
{
	enum:Uint32
	{
		DTB_MAGIC=0xD00DFEED
	};
	
	Uint32 Magic,
		   TotalSize,//Eidian??
		   OffsetDTD,
		   OffsetDTS,
		   OffsetMRM,
		   Version,
		   LastCompVersion,
		   BootPhysicalCPUID,
		   SizeDTS,
		   SizeDSD;
	
	inline bool Valid() const
	{return Magic==DTB_MAGIC;}
	
	const char* Str(Uint32 addr) const
	{return (char*)this+POS::BigEndianToThis(OffsetDTS)+addr;}
};

struct DTB_MemoryReserveMap
{
	Uint64 Address,
		   Size;
};

struct DTB_DeviceTreeData
{
	enum:Uint32
	{
		DTB_BEGIN_NODE	=1,
		DTB_END_NODE	=2,
		DTB_PROP		=3,
		DTB_NOP			=4,
		DTB_END			=9
	};
	
	
};

inline char* PrintDeviceTreeNode(DTB_BootParamHeader *dtb,char *p,int dep=0,bool nop=0)
{
	using namespace POS;
	auto TabName=[dep](const char *str)
	{
		for (int i=0;i<dep;++i)
			kout<<"  ";
		kout<<str;
	};
	auto ValueStr=[](char *&p)
	{
		while (*p!=0)
			kout<<*p++;
		while ((PtrInt)p&0x3)
			++p;
	};
	auto ValueStrNop=[](char *&p)
	{
		while (*p!=0)
			++p;
		while ((PtrInt)p&0x3)
			++p;
	};
	
	p+=4;
	if (!nop)
	{
		TabName("Node:");
		ValueStr(p);
		kout<<"\n";
	}
	else ValueStrNop(p);
	while (1)
		switch (BigEndianToThis(*(int*)p))
		{
			case DTB_DeviceTreeData::DTB_BEGIN_NODE:
				p=PrintDeviceTreeNode(dtb,p,dep+1,nop);
				break;
			case DTB_DeviceTreeData::DTB_END_NODE:
				if (!nop)
					TabName("Node End\n");
				return p+4;
			case DTB_DeviceTreeData::DTB_PROP:
			{
				Uint32 len=BigEndianToThis(*(int*)(p+=4));
				Uint32 nameoff=BigEndianToThis(*(int*)(p+=4));
				p+=4;
				if (!nop)
				{
					TabName("Property:");
					kout<<dtb->Str(nameoff)<<"\n";
					TabName("Value   :");
					kout<<DataWithSize(p,len)<<"\n";
					TabName("ValueStr:");
					for (int i=0;i<len;++i)
						kout.operator << <char>(p[i]);
					kout<<"\n";
				}
				p+=len;
				break;
			}
			case DTB_DeviceTreeData::DTB_NOP:
				nop=1;
				p+=4;
				break;
			case DTB_DeviceTreeData::DTB_END:
				return p+4;
			default:
				kout[Error]<<"DeviceTreeBlob unknown Tag "<<BigEndianToThis(*(int*)p)<<endl;
				return nullptr;
		}
}

inline void PrintDeviceTree(void *addr)
{
	using namespace POS;
	kout[Debug]<<addr<<endl;
	DTB_BootParamHeader *dtb=(DTB_BootParamHeader*)addr;
	if (!dtb->Valid())
	{
		kout[Error]<<"DeviceTreeBlob addr "<<addr<<" is not valid, magic "<<DataWithSize(&dtb->Magic,4)<<endl;
		return;
	}
	kout[Info]<<"PrintDeviceTree:"<<endl;
	PrintDeviceTreeNode(dtb,(char*)dtb+BigEndianToThis(dtb->OffsetDTD));
	kout[Info]<<"PrintDeviceTree OK"<<endl;
}

#endif
