#!/bin/bash
# If build directory doesn't exist, configure the project
if [ ! -d "build" ]; then
    # Check if ninja is available
    GENERATOR_ARGS=""
    if command -v ninja >/dev/null 2>&1; then
        GENERATOR_ARGS="-G Ninja"
    fi

    cmake -B build $GENERATOR_ARGS \
      -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake
fi

# Execute build
cmake --build build
