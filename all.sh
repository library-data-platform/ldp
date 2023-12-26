#!/bin/sh
set -e
set -x
./version.sh
mkdir -p build
cd build
cmake -DPostgreSQL_TYPE_INCLUDE_DIR=/usr/include/postgresql ..
make -j 4
