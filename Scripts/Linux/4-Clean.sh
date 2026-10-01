#!/bin/bash
set -e

cd "$(dirname "${BASH_SOURCE[0]}")/../.."

rm -rf "build"
rm -rf "_Bin"
rm -rf "_Build"
rm -rf "_Shaders"
rm -rf "_NRI_SDK"
