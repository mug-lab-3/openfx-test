#!/bin/bash
# This is a legacy entry point. 
# Building via Docker is now the recommended way.

if [[ "$1" == "--local" ]]; then
    echo "Running local build (requires local LLVM/MinGW/SDKs)..."
    mkdir -p build
    cmake -B build -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake
    cmake --build build
else
    echo "Delegating to docker-build.sh..."
    ./docker-build.sh
fi
