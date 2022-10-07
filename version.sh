#!/bin/sh
echo \#define LDPVERSION \"`git describe --tags --always`\" > src/version.h
