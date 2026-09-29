#!/bin/bash
set -e

cd "$(dirname "${BASH_SOURCE[0]}")/../.."

SDK=_NRI_SDK

rm -rf "${SDK}"
mkdir -p "${SDK}/Include" "${SDK}/Lib/Debug" "${SDK}/Lib/Release"

cp -R Include/. "${SDK}/Include"
cp LICENSE.txt README.md nri.natvis "${SDK}"

cp -L _Bin/Debug/libNRI.dylib "${SDK}/Lib/Debug"
cp -L _Bin/Release/libNRI.dylib "${SDK}/Lib/Release"

# Metal Shader Converter library, if NRI is built with it
for CONFIG in Debug Release; do
    if [ -f "_Bin/${CONFIG}/libmetalirconverter.dylib" ]; then
        cp -L "_Bin/${CONFIG}/libmetalirconverter.dylib" "${SDK}/Lib/${CONFIG}"
    fi
done

if [ -f "${SDK}/Lib/Release/libmetalirconverter.dylib" ]; then
    echo "${SDK}: 'libmetalirconverter.dylib' is distributed under Apple's Metal Shader Converter license terms (see README.md)"
fi
