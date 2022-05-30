mkdir -p ./Build/Kernel
mkdir -p ./Build/User
cd ../..

riscv64-unknown-elf-g++ -w -nostdlib -fno-exceptions -fno-rtti -c Kernel/Boot/main.cpp                  -o Build/Kernel/main.o              -I"Include" -mcmodel=medany 
riscv64-unknown-elf-g++ -w -nostdlib -fno-exceptions -fno-rtti -c Kernel/Boot/Start.S                   -o Build/Kernel/Start.o             -I"Include" -mcmodel=medany 
riscv64-unknown-elf-g++ -w -nostdlib -fno-exceptions -fno-rtti -c Kernel/Boot/SystemInfo.cpp            -o Build/Kernel/SystemInfo.o        -I"Include" -mcmodel=medany 
riscv64-unknown-elf-g++ -w -nostdlib -fno-exceptions -fno-rtti -c Kernel/Trap/Clock.cpp                 -o Build/Kernel/Clock.o             -I"Include" -mcmodel=medany 
riscv64-unknown-elf-g++ -w -nostdlib -fno-exceptions -fno-rtti -c Kernel/Trap/Trap.cpp                  -o Build/Kernel/Trap.o              -I"Include" -mcmodel=medany 
riscv64-unknown-elf-g++ -w -nostdlib -fno-exceptions -fno-rtti -c Kernel/Trap/TrapEntry.S               -o Build/Kernel/TrapEntry.o         -I"Include" -mcmodel=medany 
riscv64-unknown-elf-g++ -w -nostdlib -fno-exceptions -fno-rtti -c Kernel/Trap/Syscall.cpp               -o Build/Kernel/Syscall.o           -I"Include" -mcmodel=medany 
riscv64-unknown-elf-g++ -w -nostdlib -fno-exceptions -fno-rtti -c Kernel/Memory/PhysicalMemory.cpp      -o Build/Kernel/PhysicalMemory.o    -I"Include" -mcmodel=medany 
riscv64-unknown-elf-g++ -w -nostdlib -fno-exceptions -fno-rtti -c Kernel/Memory/VirtualMemory.cpp       -o Build/Kernel/VirtualMemory.o     -I"Include" -mcmodel=medany 
riscv64-unknown-elf-g++ -w -nostdlib -fno-exceptions -fno-rtti -c Kernel/Process/Process.S              -o Build/Kernel/Process_S.o         -I"Include" -mcmodel=medany 
riscv64-unknown-elf-g++ -w -nostdlib -fno-exceptions -fno-rtti -c Kernel/Process/Process.cpp            -o Build/Kernel/Process.o           -I"Include" -mcmodel=medany 
riscv64-unknown-elf-g++ -w -nostdlib -fno-exceptions -fno-rtti -c Kernel/Library/Kout.cpp               -o Build/Kernel/Kout.o              -I"Include" -mcmodel=medany 
riscv64-unknown-elf-g++ -w -nostdlib -fno-exceptions -fno-rtti -c Kernel/Library/String/Convert.cpp     -o Build/Kernel/Convert.o           -I"Include" -mcmodel=medany 
riscv64-unknown-elf-g++ -w -nostdlib -fno-exceptions -fno-rtti -c Kernel/Library/String/StringTools.cpp -o Build/Kernel/StringTools.o       -I"Include" -mcmodel=medany 
riscv64-unknown-elf-g++ -w -nostdlib -fno-exceptions -fno-rtti -c Kernel/Library/KernelMonitor.cpp      -o Build/Kernel/KernelMonitor.o     -I"Include" -mcmodel=medany 
riscv64-unknown-elf-g++ -w -nostdlib -fno-exceptions -fno-rtti -c Kernel/File/FileSystem.cpp            -o Build/Kernel/FileSystem.o        -I"Include" -mcmodel=medany 
riscv64-unknown-elf-g++ -w -nostdlib -fno-exceptions -fno-rtti -c Kernel/Library/libcpp.cpp             -o Build/Kernel/libcpp.o            -I"Include" -mcmodel=medany 
riscv64-unknown-elf-g++ -w -nostdlib -fno-exceptions -fno-rtti -c Kernel/File/FAT32.cpp                 -o Build/Kernel/FAT32.o            -I"Include" -mcmodel=medany 

riscv64-unknown-elf-gcc -w -nostdlib -fno-exceptions -fno-rtti -c Kernel/HAL/Drivers/_dmac.c            -o Build/Kernel/_dmac.o             -I"Include" -mcmodel=medany 
riscv64-unknown-elf-gcc -w -nostdlib -fno-exceptions -fno-rtti -c Kernel/HAL/Drivers/_plic.c            -o Build/Kernel/_plic.o             -I"Include" -mcmodel=medany 
riscv64-unknown-elf-gcc -w -nostdlib -fno-exceptions -fno-rtti -c Kernel/HAL/Drivers/_spi.c             -o Build/Kernel/_spi.o              -I"Include" -mcmodel=medany 
riscv64-unknown-elf-gcc -w -nostdlib -fno-exceptions -fno-rtti -c Kernel/HAL/Drivers/_sysctl.c          -o Build/Kernel/_sysctl.o           -I"Include" -mcmodel=medany 
riscv64-unknown-elf-gcc -w -nostdlib -fno-exceptions -fno-rtti -c Kernel/HAL/Drivers/_gpiohs.c          -o Build/Kernel/_gpiohs.o           -I"Include" -mcmodel=medany 
riscv64-unknown-elf-gcc -w -nostdlib -fno-exceptions -fno-rtti -c Kernel/HAL/Drivers/_fpioa.c           -o Build/Kernel/_fpioa.o            -I"Include" -mcmodel=medany 
riscv64-unknown-elf-g++ -w -nostdlib -fno-exceptions -fno-rtti -c Kernel/HAL/Drivers/DriverTools.cpp    -o Build/Kernel/DriverTools.o       -I"Include" -mcmodel=medany 
riscv64-unknown-elf-g++ -w -nostdlib -fno-exceptions -fno-rtti -c Kernel/HAL/Drivers/_sdcard.c          -o Build/Kernel/_sdcard.o           -I"Include" -mcmodel=medany 

rm Build/Kernel.img
riscv64-unknown-elf-ld -o Build/Kernel/kernel.elf -T Linker/kernel.ld Build/Kernel/*.o --format=binary Build/*.img --format=default
riscv64-unknown-elf-objcopy Build/Kernel/kernel.elf --strip-all -O binary Build/Kernel.img

riscv64-unknown-elf-objcopy ../../SBI/rustsbi-k210 --strip-all -O binary ./os.bin
dd if=./Build/Kernel.img of=./os.bin bs=128k seek=1

cd Tools/Scripts