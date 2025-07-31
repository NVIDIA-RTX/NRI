#!/bin/bash
set -e

mkdir -p "_Build"

cd "_Build"
cmake ..
cd ..
