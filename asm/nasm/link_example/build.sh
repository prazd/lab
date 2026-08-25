#!/bin/bash
nasm -f macho64 add.asm -o add.o
c++-15 -std=c++23 main.cpp add.o
rm add.o
