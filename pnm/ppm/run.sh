#!/bin/bash

c++-15 -std=c++23 main.cpp
./a.out
ffmpeg -framerate 20 -start_number 0 -i res-%d.ppm -c:v libx264 -pix_fmt yuv420p out.mp4
rm res*
ffplay -loop 0  -window_title "GoL" out.mp4
