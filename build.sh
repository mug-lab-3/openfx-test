#!/bin/bash
# If build directory doesn't exist, configure the project
if [ ! -d "build" ]; then
    # Check if ninja is available
    GENERATOR_ARGS=""
    if command -v ninja >/dev/null 2>&1; then
        GENERATOR_ARGS="-G Ninja"
    fi

    cmake -B build $GENERATOR_ARGS \
      -DCMAKE_CXX_COMPILER=clang++ \
      -DCMAKE_CXX_FLAGS="-target x86_64-pc-windows-gnu" \
      -DCMAKE_SYSTEM_NAME=Windows
fi

# Execute build
cmake --build build
