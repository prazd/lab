#!/bin/bash
make NAME=$1
rm -rf $1
mkdir $1
mv "$1.ko" "$1.mod"* "$1.o"  $1
insmod "$1/$1.ko"
