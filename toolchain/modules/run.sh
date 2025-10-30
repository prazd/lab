#!/bin/bash
# https://github.com/JRASoftware/cpp23-import-std-guide
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -G Ninja
cmake --build build
./build/Main
