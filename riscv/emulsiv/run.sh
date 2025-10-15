#!/bin/bash

SRC=$1
OUT=$2

# ./run.sh risc_V_hello.c result

riscv64-unknown-elf-gcc -march=rv32i -mabi=ilp32 -ffreestanding -c -o ./build/main.o ${SRC}
riscv64-unknown-elf-gcc -march=rv32i -mabi=ilp32 -c -o ./build/startup.o ./lib/startup.s
riscv64-unknown-elf-gcc -march=rv32i -mabi=ilp32 -nostdlib -T ./lib/emulsiv.ld -o ./build/${OUT}.elf ./build/startup.o ./build/main.o
riscv64-unknown-elf-objcopy -O ihex ./build/${OUT}.elf ./build/${OUT}.hex
