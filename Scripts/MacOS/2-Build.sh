#!/bin/bash
set -e

cd "$(dirname "${BASH_SOURCE[0]}")/../.."

cmake --build _Build --config Release --parallel
cmake --build _Build --config Debug --parallel
