clean:
	rm -f exe *.o *.ld

build:

run:
	qemu-system-riscv64 -machine virt -m 256M -nographic -bios default -kernel kernel.elf
