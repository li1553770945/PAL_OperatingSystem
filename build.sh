riscv64-unknown-linux-gnu-gcc -nostdlib -c entry.s -o entry.o
riscv64-unknown-linux-gnu-gcc -nostdlib -c main.c -o main.o -mcmodel=medany
riscv64-unknown-linux-gnu-ld -o kernel.elf -Tlink.ld entry.o main.o

