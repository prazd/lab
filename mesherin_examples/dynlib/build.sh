#!/bin/bash

g++ -std=c++20 -shared myfunc.cpp -o libmyfunc.dylib
g++ -std=c++20 main.cpp -lmyfunc -L.
