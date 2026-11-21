#!/bin/bash
c++ -std=c++23 main.cpp -o main
c++ -std=c++23 cli.cpp -o cli
sudo setcap cap_sys_ptrace=eip ./cli
