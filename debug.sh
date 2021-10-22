#!/bin/sh
set -e
set -x
mkdir -p build
cd build
cmake -DCMAKE_RULE_MESSAGES=OFF -DDEBUG=ON -DOPTIMIZE=OFF -DPostgreSQL_TYPE_INCLUDE_DIR=/usr/include/postgresql ..
make -j 4 ldp
