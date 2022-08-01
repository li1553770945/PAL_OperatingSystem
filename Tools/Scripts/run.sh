cd ../..

echo "=====================PAL_OperatingSystem========================="
#qemu-system-riscv64 -smp 2 -machine virt -m 128M -nographic -bios SBI/rustsbi-qemu -drive file=Bin/3.vhd,if=none,format=raw,id=x0 -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0 -device loader,file=Build/Kernel.img,addr=0x80200000
qemu-system-riscv64 -machine virt -kernel Build/Kernel/kernel.elf -m 128M -nographic -smp 2 -bios SBI/rustsbi-qemu -drive file=Bin/3.vhd,if=none,format=raw,id=x0  -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0 -initrd Bin/4.vhd

cd Tools/Scripts