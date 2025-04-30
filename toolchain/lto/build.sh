#!/bin/bash
g++ -std=c++2b -O2 main.cpp f.cpp -o build/a.out
g++ -std=c++2b -O2 -flto main.cpp f.cpp  -o build/lto_a.out
