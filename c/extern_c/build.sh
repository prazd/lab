#!/bin/bash

g++ -c -std=c++20 func.cpp -o func.o
gcc -c  main.c -o main.o
g++ main.o func.o
rm main.o func.o
