#!/bin/bash

c++-15 -std=c++23 -shared print.cpp -o libprint.dylib
c++-15 -O2 -std=c++23 main.cpp
#g++ -fsanitize=undefined -std=c++2b main.cpp

