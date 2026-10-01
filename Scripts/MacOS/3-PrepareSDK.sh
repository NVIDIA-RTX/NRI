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
