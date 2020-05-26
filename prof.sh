#!/bin/sh
set -e
set -x
mkdir -p build
cd build
cmake -DGPROF=ON -DDEBUG=ON -DOPTIMIZE=OFF ..
make
./src/test_ldp

