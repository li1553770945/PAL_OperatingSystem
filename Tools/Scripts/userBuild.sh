cd ../..

for file in User/Library/*{.cpp,.S}
do
	namean=${file##*/}
	name=${namean%%.*}
	riscv64-unknown-elf-g++ -nostdlib -c $file -o Build/User/$name.o -I"Include" -mcmodel=medany 
done

for file in User/*.cpp
do
	namean=${file##*/}
	name=${namean%%.cpp}
	riscv64-unknown-elf-g++ -nostdlib -c $file -o Build/User/$name.o -I"Include" -I"User/Library" -mcmodel=medany 
	riscv64-unknown-elf-ld -o Build/User/$name.elf -T Linker/user.ld Build/User/$name.o Build/User/UserMain.o Build/User/UserStart.o
	riscv64-unknown-elf-objcopy Build/User/$name.elf --strip-all -O binary Build/$name.img
done

cd Tools/Scripts