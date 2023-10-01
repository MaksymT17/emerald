cd aquamarine &&
./build.sh &&
cd ../ &&
rm -rf build/ &&
mkdir -p build &&
cd build/ &&
cmake .. &&
make -j4