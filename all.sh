#!/bin/sh
set -e
set -x
mkdir -p build
cd build
cmake -DPostgreSQL_TYPE_INCLUDE_DIR=/usr/include/postgresql ..
make -j 6
