clean:
	rm -f exe *.o *.ld

build:
	riscv64-unknown-linux-gnu-gcc -nostdlib -c start.s -o start.o
	riscv64-unknown-linux-gnu-gcc -nostdlib -c kernel/main.c -o main.o -mcmodel=medany
	riscv64-unknown-linux-gnu-ld -o kernel.elf -Tlink.ld start.o main.o

run:
	qemu-system-riscv64 -machine virt -m 256M -nographic -bios default -kernel kernel.elf
