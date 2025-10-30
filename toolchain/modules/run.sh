#!/bin/bash
# TODO: finish
# https://github.com/JRASoftware/cpp23-import-std-guide
cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=c++-15 -DCMAKE_C_COMPILER=gcc-15
cmake --build build
./build/Main
