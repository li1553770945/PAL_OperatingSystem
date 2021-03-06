
K=Kernel
L=Linker
B=Build
T=Test
U=User

TOOLPREFIX=riscv64-unknown-elf-
CXX=$(TOOLPREFIX)g++ #编译器，会自动使用这个根据.o文件列表编译.c文件
LD=$(TOOLPREFIX)ld
AS=$(CXX)
INC_DIR=Include

CXXFLAGS=-c -nostdlib  -mcmodel=medany  -fno-exceptions -fno-rtti #编译选项
LDFLAGS=  #连接选项

SOURCE=$(wildcard ./*cpp  $(K)/Boot/*.cpp $(K)/Library/*.cpp $(K)/Library/String/*.cpp $(K)/Memory/*.cpp $(K)/Process/*.cpp \
$(K)/Trap/*.cpp $(K)/File/*.cpp $(T)/*.cpp)  

USER_SOURCE = $(wildcard $(U)/*.cpp $(U)/Library/*.cpp)
AS_SOURCE=

OBJ_DIR = Build
INCLUDE = -I Include
OBJECTS=$(patsubst %.cpp,%.o,$(SOURCE))
USER_OBJECTS=$(patsubst %.cpp,%.o,$(USER_SOURCE))

%.o:%.cpp
	$(CXX) $(INCLUDE) $(CXXFLAGS) $< -o $(B)/$(notdir $@) 

as:
	$(CXX) $(INCLUDE) $(CXXFLAGS) $(K)/Boot/Start.S -o $(B)/Start.o
	$(CXX) $(INCLUDE) $(CXXFLAGS) $(K)/Trap/TrapEntry.S -o $(B)/TrapEntry.o
	$(CXX) $(INCLUDE) $(CXXFLAGS) $(K)/Process/Process.S -o $(B)/Process_S.o


dir:
	@if [ ! -d "./$(B)" ]; then mkdir $(B); fi

user:
	./Tools/Scripts/userBuild.sh

build: dir user $(OBJECTS) as
	$(LD) $(LDFLAGS) -o $(B)/kernel.elf -T$(L)/link.ld $(wildcard $(B)/*.o)  $(LDFLAGS)

all: 
	bash ./Tools/Scripts/all.sh

QEMU=qemu-system-riscv64
QEMUFLAGS=-machine virt -m 128M -nographic -bios SBI/rustsbi-qemu -device loader,file=Build/kernel.bin,addr=0x80200000
run:dir build
	riscv64-unknown-elf-objcopy -O binary Build/kernel.elf Build/kernel.bin
	
	$(QEMU) $(QEMUFLAGS)


clean:
	rm -f  $(B)/* os.bin

