#!/bin/bash

ROOT=$(pwd)
SELF=$(dirname "$0")

rm -rf "_NRI_SDK"
mkdir -p "_NRI_SDK"
cd "_NRI_SDK"

mkdir -p "Include"
mkdir -p "Lib/Debug"
mkdir -p "Lib/Release"

cp -r "$(SELF)/Include/" "Include"
cp "$(SELF)/LICENSE.txt" "."
cp "$(SELF)/README.md" "."
cp "$(SELF)/nri.natvis" "."

cp -H "$(ROOT)/_Bin/Debug/libNRI.so" "Lib/Debug"
cp -H "$(ROOT)/_Bin/Release/libNRI.so" "Lib/Release"

cd ..

