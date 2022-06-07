# ELF文件解析

#### ELF文件介绍

​		ELF文件是Linux的可执行文件格式，它适用性广泛，定义良好，易于解析，我们在实现解析的过程中确实感觉比较好处理。ELF文件分链接视图和执行视图，链接视图关心节和符号表，而执行视图关心段表，我们只需要解析它并在我们的系统上执行，因此只需要实现段表相关的解析即可。



#### ELF文件格式

​		ELF文件由EFL头、程序头表、各程序段组成。

其中ELF头表实现时我们采用了模板，以同时适配32位和64位。具体如下：

````c++
template <class AddrType> struct ELF_HeaderXX
{
	union
	{
		Uint8 ident[16];//Magic number and other info
		struct
		{
			Uint8 magic[4];
			Uint8 elfClass,
				  dataEndian,
				  elfVersion;
			Uint8 padding[9];
		};
	};
	Uint16 type;//Object file type
	Uint16 machine;//Architecture
	Uint32 version;//Object file version
	AddrType entry;//Entry point virtual address
	AddrType phoff;//Program header table file offset
	AddrType shoff;//Section header table file offset
	Uint32 flags;//Processor-specific flags
	Uint16 ehsize;//ELF header size in bytes
	Uint16 phentsize;//Program header table entry size
	Uint16 phnum;//Program header table entry count
	Uint16 shentsize;//Section header table entry size
	Uint16 shnum;//Section header table entry count
	Uint16 shstrndx;//Section header string table index
	
	inline bool IsELF() const
	{return magic[0]==0x7F&&magic[1]=='E'&&magic[2]=='L'&&magic[3]=='F';}
}__attribute__((packed));
````

程序头表的布局32位和64位差异略大，因此需要分两种来写，此处只展示64位版本，特别注意32位版本的flags域所处位置不同

````c++
struct ELF_ProgramHeader64
{
	enum
	{
		PT_NULL=0,//Empty segment
		PT_LOAD=1,//Loadable segment
		PT_DYNAMIC=2,//Segment include dynamic linker info
		PT_INTERP=3,//Segment specified dynamic linker
		PT_NOTE=4,//Segment include compiler infomation
		PT_SHLIB=5,//Shared library segment
		PT_LOPROC=0x70000000,
		PT_HIPROC=0x7fffffff
	};
	
	enum
	{
		PF_X=1,
		PF_W=2,
		PF_R=4,
		PF_MASKPROC=0xf0000000
	};
	
	Uint32 type;//Segment type
	Uint32 flags;
	Uint64 offset;//Segment file offset
	Uint64 vaddr;//Segment virtual address
	Uint64 paddr;//Segment physical address(used in system without virtual memory)
	Uint64 filesize;//Segment size in file
	Uint64 memsize;//Segment size in memory
	Uint64 align;//Segment alignment
}__attribute__((packed));
````



#### 通过ELF文件创建进程

​		在我们的实现中，通过ELF文件创建进程的大致流程如下：

1. 通过FileHandle读取ELF文件头表，如果不是ELF文件则返回错误
2. 像建立新进程一样创建VMS和进程对象，并设置相关参数
3. 设置进程的入口为ELF文件指定的入口，并插入内核预处理函数`Thread_CreateProcessFromELF`
4. 等待内核预处理函数完成
5. 内核预处理函数读出所有段，根据段的信息创建对应VMR，并从文件拷贝所需数据
6. 设置栈和堆段的VMR
7. 唤醒步骤4等待的进程，然后继续执行会进入用户进程部分



------------------------

By：qianpinyi

2022.06.05