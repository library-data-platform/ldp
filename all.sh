#!/bin/sh
set -e
set -x
mkdir -p build
cd build
cmake ..
make
#./ldp_test

