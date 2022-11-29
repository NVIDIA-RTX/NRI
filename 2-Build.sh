#!/bin/bash

mkdir -p "_Compiler"

cd "_Compiler"
rm CMakeCache.txt
cmake .. -A x64
cmake --build . --config Release
cd ..

read -p "Do you want to compile DEBUG configuration? [y/n]" -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]
then
    cd "_Compiler"
    rm CMakeCache.txt
    cmake .. -A x64
    cmake --build . --config Debug
    cd ..
fi
