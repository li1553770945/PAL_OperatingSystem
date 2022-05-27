OUTPUT_ARCH("riscv")
ENTRY(_start)

SECTIONS
{
	. = 0x800020;
	
	.text :
	{
		textstart = .;
		*(.text.entry)
		*(.text .text.*)
		textend = .;
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
		dataend = .;
	}

	.bss :
	{
		bssstart = .;
		*(.sbss .sbss.* .bss .bss.*)
		bssend = .;
	}
	
	. = ALIGN(4k);
}