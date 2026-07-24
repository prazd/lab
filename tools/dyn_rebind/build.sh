#/bin/bash

c++-15 -std=c++23 -g -shared mmapinfo.cpp -o libmmapinfo.dylib
c++-15 -std=c++23 main.cpp
