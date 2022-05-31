OUTPUT_ARCH("riscv")
ENTRY(_start)
SECTIONS
{
	. = 0xffffffffc0020000;
	
	PROVIDE(kernelstart = .);
	
	.text :
	{
		PROVIDE(textstart = .);
		*(.text.entry)
		*(.text .text.*)
		PROVIDE(textend = .);
	}
	
	.rodata : 
	{
		PROVIDE(rodatastart = .);
		*(.srodata .srodata.* .rodata .rodata.*)
		PROVIDE(rodataend = .);
	}
	. = ALIGN(4k);

	.data :
	{
		PROVIDE(datastart = .);
		*(.data .data.*)
		PROVIDE(dataend = .);
	}

	.bss :
	{
		PROVIDE(bssstart = .);
		*(.sbss .sbss.* .bss .bss.*)
		PROVIDE(bssend = .);
	}
	
	PROVIDE(kernelend = .);
	. = ALIGN(4k);
	PROVIDE(freememstart = .);
}