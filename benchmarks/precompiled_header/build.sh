#!/bin/bash

build() {
    for file in main m1 m2; do
        echo "Build $file.cpp"
        "$@" -c $file.cpp
        rm ${file}.o
        echo ""
    done    
}
echo "---------------Default version-----------------"
build time c++-14 -std=c++20
echo "---------Precompiled headers version-----------"
c++-14 -std=c++20 -c stdafx.h
build time c++-14 -include stdafx.h -std=c++20
rm stdafx.h.gch
