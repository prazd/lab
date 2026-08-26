#!/bin/bash
nasm -f macho64 coro.asm -o coro.o
c++-15 -g -std=c++23 main.cpp coro.o
rm coro.o
