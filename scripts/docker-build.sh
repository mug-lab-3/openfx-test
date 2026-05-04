#!/bin/bash
set -e

# Ensure we are in the repository root
cd "$(dirname "$0")/.."

# Handle clean build option
if [[ "$1" == "--clean" ]]; then
    echo "Cleaning build and cache directories..."
    rm -rf build .ccache
fi

IMAGE_NAME="resolve-ofx-win64-builder"

# Build the image (generic)
echo "Ensuring Docker image is ready..."
docker build -t $IMAGE_NAME -f docker/Dockerfile .

# Run the build inside the container
# --user $(id -u):$(id -g) makes files owned by you on the host.
# We set HOME to /tmp because the container doesn't have your host home dir.
mkdir -p .ccache build
docker run --rm \
    --user $(id -u):$(id -g) \
    -v "$(pwd):/workspace" \
    -v "$(pwd)/.ccache:/tmp/.ccache" \
    -e HOME=/tmp \
    -e CCACHE_DIR=/tmp/.ccache \
    $IMAGE_NAME

# Fix paths in compile_commands.json for editor support
if [ -f build/compile_commands.json ]; then
    echo "Fixing paths in compile_commands.json for host editor..."
    sed -i "s|/opt/ofx-sdk|$(pwd)/.sdk/ofx-sdk|g" build/compile_commands.json
    sed -i "s|/opt/3rdparty|$(pwd)/.sdk|g" build/compile_commands.json
    sed -i "s|/workspace|$(pwd)|g" build/compile_commands.json
fi

echo "Build complete. Output: build/MugPlugin.ofx"
