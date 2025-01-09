#!/bin/bash

g++ -shared myfunc.cpp -o libmyfunc.dylib
g++ main.cpp -lmyfunc -L.
