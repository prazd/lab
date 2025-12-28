#!/bin/bash
cmake -S . -B build
cmake --build build
./build/Main
ffmpeg -framerate 30 -start_number 0 -i res-%d.ppm -c:v libx264 -pix_fmt yuv420p out.mp4
rm res*
ffplay -loop 0  -window_title "Main" out.mp4
