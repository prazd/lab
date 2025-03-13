#!/bin/bash

g++ -std=c++20 -shared verify.cpp -o libverify.dylib
g++ -std=c++20 main.cpp -lverify -L.