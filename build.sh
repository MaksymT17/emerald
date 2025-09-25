rm -rf silber/
git clone --branch "1.2" --depth 1 https://github.com/MaksymT17/silber &&
mkdir -p build &&
cd build/ &&
cmake .. &&
make -j4