#!/bin/sh
set -e
set -x
mkdir -p build
cd build
cmake -DDEBUG=ON -DOPTIMIZE=OFF ..
make -j 4 ldp
