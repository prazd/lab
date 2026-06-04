#!/bin/bash

gcc -g main.c -o d_out
#g++ -I include -std=c++2b  main.cpp
c++-15 -I include -O3 -std=c++23 main.cpp
