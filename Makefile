
K=kernel
T=target
L=linker
TOOLPREFIX=riscv64-unknown-linux-gnu-

clean:
	rm -f *.o $T/*

build:
	$(TOOLPREFIX)gcc -nostdlib -c start.s -o $T/start.o
	$(TOOLPREFIX)gcc -nostdlib -c $K/printk.c -o $T/printk.o
	$(TOOLPREFIX)gcc -nostdlib -c $K/main.c -o $T/main.o -mcmodel=medany
	$(TOOLPREFIX)ld -o $T/kernel.elf -T$L/link.ld $T/start.o $T/main.o $T/printk.o

run:
	qemu-system-riscv64 -machine virt -m 256M -nographic -bios default -kernel $T/kernel.elf
