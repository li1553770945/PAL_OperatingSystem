cd ../..

mkdir Submit
cd Submit

for file in *
do
	if [ "$file" != ".git" ]
	then
		rm -r $file
	fi
done

cd ..

cp -r -p Doc Submit/Doc
cp -r -p Include Submit/Include
cp -r -p Kernel Submit/Kernel
cp -r -p Linker Submit/Linker
cp -r -p SBI Submit/SBI
cp -r -p Tools Submit/Tools
cp -r -p User Submit/User
cp -r -p License.md Submit/License.md
cp -r -p LICENSE Submit/LICENSE
cp -r -p Makefile Submit/Makefile
cp -r -p README.md Submit/README.md

cd Tools/Scripts