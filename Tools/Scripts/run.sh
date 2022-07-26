cd ../..

echo "=====================PAL_OperatingSystem========================="
qemu-system-riscv64 -smp 2 -machine virt -m 6M -nographic -bios SBI/rustsbi-qemu -drive file=Bin/3.vhd,if=none,format=raw,id=x0 -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0 -device loader,file=Build/Kernel.img,addr=0x80200000

cd Tools/Scripts