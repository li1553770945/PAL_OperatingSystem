OUTPUT_ARCH("riscv")
ENTRY(_start)
SECTIONS
{
	. = 0xffffffffc0020000;
	
	PROVIDE(kernelstart = .);
	
	.text :
	{
		textstart = .;
		*(.text.entry)
		*(.text .text.*)
		PROVIDE(textend = .);
	}
	
	.rodata : 
	{
		rodatastart = .;
		*(.srodata .srodata.* .rodata .rodata.*)
		rodataend = .;
	}
	. = ALIGN(4k);

	.data :
	{
		datastart = .;
		*(.data .data.*)
		PROVIDE(dataend = .);
	}

	.bss :
	{
		bssstart = .;
		*(.sbss .sbss.* .bss .bss.*)
		bssend = .;
	}
	
	PROVIDE(kernelend = .);
	. = ALIGN(4k);
	PROVIDE(freememstart = .);
}