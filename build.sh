#!/bin/bash
# If build directory doesn't exist, configure the project
if [ ! -d "build" ]; then
    cmake -B build \
      -DCMAKE_CXX_COMPILER=clang++ \
      -DCMAKE_CXX_FLAGS="-target x86_64-pc-windows-gnu" \
      -DCMAKE_SYSTEM_NAME=Windows
fi

# Execute build
cmake --build build
