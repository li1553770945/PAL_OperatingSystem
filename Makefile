
K=kernel
T=target
L=linker
TOOLPREFIX=riscv64-unknown-linux-gnu-
CC=$(TOOLPREFIX)gcc #编译器，会自动使用这个根据.o文件列表编译.c文件
LD=$(TOOLPREFIX)ld
CFLAGS=-nostdlib  -mcmodel=medany #编译选项
LDFLAGS=  #连接选项

OBJS=$K/main.o \ #所有的目标文件，会自动根据.o编译对应的.c文件
$K/printk.o \
$K/sbi.o 

clean:
	rm -f  $T/* $(OBJS)

start:
	$(CC) -nostdlib -c start.s -o start.o

build: $(OBJS) start
	@if [ ! -d "./target" ]; then mkdir target; fi
	$(LD) $(LDFLAGS) -o $T/kernel.elf -T$L/link.ld start.o $(OBJS) 


QEMU=qemu-system-riscv64
QEMUFLAGS=-machine virt -m 256M -nographic -bios default -kernel $T/kernel.elf
run:
	$(QEMU) $(QEMUFLAGS)
