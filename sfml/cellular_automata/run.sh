#!/bin/bash

files=()
for f in *; do
  [[ "$f" == "CmakeLists.txt" || "$f" == "build" || "$f" == "run.sh" ]] && continue
  [[ -f "$f" ]] && files+=("${f%.cpp}")
done

count=0
for i in "${files[@]}"; do
    echo "$count. ${i}"
    ((++count))
done

read -p "=>" num
if [[ ! "$num" =~ ^[0-9]+$ ]] || (( num < 0 || num >= ${#files[@]} )); then
    echo "Bad num"
    exit 1
fi
cmake -B build -DAUTOMATON_NAME="${files[num]}"
cmake --build build
./build/${files[num]}
