#!/bin/bash
make
rm -rf call_ioctl
mkdir call_ioctl
mv "call_ioctl.ko" "call_ioctl.mod"* "call_ioctl.o"  call_ioctl
insmod "call_ioctl/call_ioctl.ko"
