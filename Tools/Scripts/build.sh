riscv64-unknown-elf-g++ -nostdlib -c ../../Kernel/Boot/main.cpp                  -o ../../Build/main.o           -I"../../Include" -mcmodel=medany 
riscv64-unknown-elf-g++ -nostdlib -c ../../Kernel/Boot/Start.S                   -o ../../Build/Start.o          -I"../../Include" -mcmodel=medany 
riscv64-unknown-elf-g++ -nostdlib -c ../../Kernel/Boot/SystemInfo.cpp            -o ../../Build/SystemInfo.o     -I"../../Include" -mcmodel=medany 
riscv64-unknown-elf-g++ -nostdlib -c ../../Kernel/Trap/Clock.cpp                 -o ../../Build/Clock.o 	     -I"../../Include" -mcmodel=medany 
riscv64-unknown-elf-g++ -nostdlib -c ../../Kernel/Trap/Interrupt.cpp             -o ../../Build/Interrupt.o      -I"../../Include" -mcmodel=medany 
riscv64-unknown-elf-g++ -nostdlib -c ../../Kernel/Trap/Trap.cpp                  -o ../../Build/Trap.o           -I"../../Include" -mcmodel=medany 
riscv64-unknown-elf-g++ -nostdlib -c ../../Kernel/Trap/TrapEntry.S               -o ../../Build/TrapEntry.o      -I"../../Include" -mcmodel=medany 
riscv64-unknown-elf-g++ -nostdlib -c ../../Kernel/Library/Kout.cpp               -o ../../Build/Kout.o           -I"../../Include" -mcmodel=medany 
riscv64-unknown-elf-g++ -nostdlib -c ../../Kernel/Library/String/Convert.cpp     -o ../../Build/Convert.o        -I"../../Include" -mcmodel=medany 
riscv64-unknown-elf-g++ -nostdlib -c ../../Kernel/Library/String/StringTools.cpp -o ../../Build/StringTools.o    -I"../../Include" -mcmodel=medany 
riscv64-unknown-elf-g++ -nostdlib -c ../../Kernel/Memory/PhysicalMemory.cpp      -o ../../Build/PhysicalMemory.o -I"../../Include" -mcmodel=medany 
riscv64-unknown-elf-g++ -nostdlib -c ../../Test/TestMemory.cpp      -o ../../Build/TestMemory.o -I"../../Include" -mcmodel=medany 


riscv64-unknown-elf-ld -o ../../Build/1.elf -T ../../Linker/link.ld ../../Build/*.o && riscv64-unknown-elf-objcopy -O binary ../../Build/1.elf ../../Build/1.img