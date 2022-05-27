riscv64-unknown-elf-objcopy ../../SBI/rustsbi-k210 --strip-all -O binary ../../Build/PAL_OS.bin
dd if=../../Build/Kernel.img of=../../Build/PAL_OS.bin bs=128k seek=1
