#!/bin/sh
set -e
set -x
mkdir -p build
cd build
cmake -DCMAKE_RULE_MESSAGES=OFF -DPostgreSQL_TYPE_INCLUDE_DIR=/usr/include/postgresql ..
make
