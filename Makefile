
K=kernel
T=target
L=linker
I=include
SBI=fw_jump.elf #使用的SBI
TOOLPREFIX=riscv64-unknown-elf-
CC=$(TOOLPREFIX)gcc
LD=$(TOOLPREFIX)ld
CFLAGS=-nostdlib  -mcmodel=medany #编译选项
LDFLAGS=  #连接选项

#所有的目标文件，会自动根据.o编译对应的.c文件
OBJS=$K/main.o \
$K/printk.o \
$K/sbi.o \
$K/init.o \
$K/trap/trap.o \
$K/clock.o \
$K/driver/intr.o

# 使用汇编编译出来的o文件，如果使用默认规则会使用错误的编译器
SOBJS=start.o \
$K/trap/trapentry.o

clean:
	rm -f  $T/* $(OBJS) $(SOBJS)

sobj:# 使用汇编编译出来的o文件，如果使用默认规则会使用错误的编译器
	$(CC) -nostdlib -c start.S -o start.o
	$(CC) -nostdlib -c $K/trap/trapentry.S -o $K/trap/trapentry.o

build: $(OBJS) sobj
	@if [ ! -d "./target" ]; then mkdir target; fi
	$(LD) $(LDFLAGS) -o $T/kernel.elf -T$L/link.ld $(OBJS) $(SOBJS)


QEMU=qemu-system-riscv64
QEMUFLAGS=-machine virt -m 256M -nographic  -kernel $T/kernel.elf
run:
	$(QEMU) $(QEMUFLAGS) -bios $(SBI)
