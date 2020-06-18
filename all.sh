#!/bin/sh
set -e
set -x
mkdir -p build
cd build
cmake ..
make -j 6
./ldp_test

