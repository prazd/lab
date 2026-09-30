#!/bin/bash
make NAME=$1
mkdir -p $1
mv "$1.ko" "$1.mod"* "$1.o"  $1