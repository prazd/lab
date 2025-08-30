#!/bin/bash

gcc -shared print.c -o libprint.dylib
c++-14 -std=c++23 main.cpp
