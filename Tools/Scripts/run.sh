cd ../..

echo "##################################################################################"
qemu-system-riscv64 -smp 2 -machine virt -m 6M -nographic -bios SBI/rustsbi-qemu	-device loader,file=Build/Kernel.img,addr=0x80200000

cd Tools/Scripts