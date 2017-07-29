git clone https://github.com/sisoputnfrba/so-commons-library.git
cd so-commons-library
sudo make install

cd ..

git clone https://github.com/sisoputnfrba/ansisop-parser.git
cd ansisop-parser
cd parser
make all
sudo make install

cd ..

cd shared-leto
sudo make all
sudo make install

cd ..

cd memoria
mkdir build
gcc src/*.c -o build/memoria -lcommons -lpthread -lm -lshared-leto -lparser-ansisop

cd ..

cd kernel
mkdir build
gcc src/*.c -o build/kernel -lcommons -lpthread -lm -lshared-leto -lparser-ansisop

cd ..

cd cpu
mkdir build
gcc src/*.c -o build/cpu -lcommons -lpthread -lm -lshared-leto -lparser-ansisop

cd ..

cd filesystem
mkdir build
gcc src/*.c -o build/filesystem -lcommons -lpthread -lm -lshared-leto -lparser-ansisop

cd ..

cd consola
mkdir build
gcc src/*.c -o build/consola -lcommons -lpthread -lm -lshared-leto -lparser-ansisop