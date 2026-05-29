#!/bin/bash

gcc -g main.c -o d_out
c++-15 -I include -O2 -std=c++23 main.cpp
