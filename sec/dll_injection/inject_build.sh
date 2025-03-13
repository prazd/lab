#!/bin/bash

g++ -std=c++20 -shared dll_inject.cpp -o libinject.dylib
g++ -std=c++20 main.cpp -linject -L.
