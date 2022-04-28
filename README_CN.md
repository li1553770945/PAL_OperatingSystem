# PAL_OperatingSystem

一个极简的操作系统。

[English Document](README.md)


## 构建 && 运行

### 准备工作  
1. 安装qemu
```shell
$ wget https://download.qemu.org/qemu-4.1.1.tar.xz
$ tar xvJf qemu-4.1.1.tar.xz
$ cd qemu-4.1.1
$ ./configure --target-list=riscv32-softmmu,riscv64-softmmu
$ make -j
$ export PATH=$PWD/riscv32-softmmu:$PWD/riscv64-softmmu:$PATH
```
警告:如果在ubuntu使用 `apt-get install`,在运行时可能出现 `qemu-system-riscv64: Unable to load the RISC-V firmware` 错误。

现在可以使用 `qemu-system-riscv64 -version` 来显示qemu的版本。

2. 安装工具链

参考[riscv-gnu-toolchain](https://github.com/riscv-collab/riscv-gnu-toolchain)

### build

直接在当前文件夹运行 `make build`.

### run

直接在当前文件夹运行 `make run`.
