cd ../..

echo "=====================PAL_OperatingSystem========================="
qemu-system-riscv64 -machine virt -kernel kernel-qemu -m 128M -nographic -smp 2 -bios sbi-qemu -drive file=Bin/3.vhd,if=none,format=raw,id=x0 -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0 -serial file:os_serial_out.txt 
cd Tools/Scripts