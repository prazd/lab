#!/bin/bash

gcc -shared print.c -o libprint.dylib
c++-15 -std=c++23 main.cpp
