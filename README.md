# PAL_OperatingSystem
A simple OperatingSystem

[中文文档](README_CN.md)

## build && run

### prepare
1. use the following code to install qemu.
```shell
$ wget https://download.qemu.org/qemu-4.1.1.tar.xz
$ tar xvJf qemu-4.1.1.tar.xz
$ cd qemu-4.1.1
$ ./configure --target-list=riscv32-softmmu,riscv64-softmmu
$ make -j
$ export PATH=$PWD/riscv32-softmmu:$PWD/riscv64-softmmu:$PATH
```
WARNING:If you use `apt-get install` in ubuntu,it my cause `qemu-system-riscv64: Unable to load the RISC-V firmware` when you run the PAL_OperatingSystem.

Now you can use `qemu-system-riscv64 -version` to show the version.

2. install toolchain

Follow the [riscv-gnu-toolchain](https://github.com/riscv-collab/riscv-gnu-toolchain)

### build

You just need to run `make build`.

### run

You just need to run `make run`.
