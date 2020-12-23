#!/bin/sh
set -e
set -x
mkdir -p build
cd build
cmake -DDEBUG=ON -DOPTIMIZE=OFF -DPostgreSQL_TYPE_INCLUDE_DIR=/usr/include/postgresql ..
make -j 4 ldp
