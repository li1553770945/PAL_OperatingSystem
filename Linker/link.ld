OUTPUT_ARCH("riscv")
OUTPUT_FORMAT("elf64-littleriscv")
ENTRY(_start)
SECTIONS
{
	. = 0xffffffffc0200000;
	
	start = .;
	.text :
	{
		stext = .;
		*(.text.entry)
		*(.text .text.*)
		PROVIDE(etext = .);
	}
	
	.rodata : 
	{
		srodata = .;
		*(.rodata .rodata.*)
		erodata = .;
	}
	. = ALIGN(4k);

	.data :
	{
		sdata = .;
		*(.data .data.*)
		PROVIDE(edata = .);
	}

	.bss :
	{
		sbss = .;
		*(.bss .bss.*)
		ebss = .;
	}
	
	PROVIDE(end = .);
	. = ALIGN(4k);
	PROVIDE(freememstart = .);
}