
K=Kernel
L=Linker
B=Build
T=Test

TOOLPREFIX=riscv64-unknown-elf-
CXX=$(TOOLPREFIX)g++ #编译器，会自动使用这个根据.o文件列表编译.c文件
LD=$(TOOLPREFIX)ld
AS=$(CXX)
INC_DIR=Include

CXXFLAGS=-c -nostdlib  -mcmodel=medany #编译选项
LDFLAGS=  #连接选项

SOURCE=$(wildcard ./*cpp  $(K)/Boot/*.cpp $(K)/Library/*.cpp $(K)/Library/String/*.cpp $(K)/Memory/*.cpp $(K)/Trap/*.cpp $(T)/*.cpp) 
AS_SOURCE=

OBJ_DIR = Build
INCLUDE = -I Include
OBJECTS=$(patsubst %.cpp,%.o,$(SOURCE))

%.o:%.cpp
	$(CXX) $(INCLUDE) $(CXXFLAGS) $< -o $(B)/$(notdir $@) 

as:
	$(CXX) $(INCLUDE) $(CXXFLAGS) $(K)/Boot/Start.S -o $(B)/Start.o
	$(CXX) $(INCLUDE) $(CXXFLAGS) $(K)/Trap/TrapEntry.S -o $(B)/TrapEntry.o


dir:
	@if [ ! -d "./$(B)" ]; then mkdir $(B); fi

build: dir $(OBJECTS) as
	$(LD) $(LDFLAGS) -o $(B)/kernel.elf -T$(L)/link.ld $(wildcard $(B)/*.o)  $(LDFLAGS)

all: build
	riscv64-unknown-elf-objcopy -O binary Build/kernel.elf os.bin

QEMU=qemu-system-riscv64
QEMUFLAGS=-machine virt -m 256M -nographic -bios SBI/opensbi_qemu.elf -device loader,file=Build/kernel.bin,addr=0x80200000
run:build
	riscv64-unknown-elf-objcopy -O binary Build/kernel.elf Build/kernel.bin
	
	$(QEMU) $(QEMUFLAGS)


clean:
	rm -f  $(B)/* os.bin

