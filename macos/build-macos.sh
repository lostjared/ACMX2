#!/bin/sh

git clone https://github.com/lostjared/libmx2.git
cd libmx2/libmx
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_RPATH="/usr/local/lib"
make -j$(sysctl -n  hw.ncpu) && sudo make install
cd ../../../
git clone https://github.com/lostjared/ACMX2.git
cd ACMX2
cd MXWrite
mkdir build && cd build
cmake ..
make -j$(sysctl -n hw.ncpu) && sudo make install
cd ..
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_RPATH="/usr/local/lib"
make -j$(sysctl -n hw.ncpu) && sudo make install
cd ../interface
qmake6 && make -j$(sysctl -n hw.ncpu)
echo "done use from this directory"
pwd
echo "./interface"

