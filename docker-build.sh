#!/bin/bash
set -e

IMAGE_NAME="resolve-ofx-win64-builder"

# Build the docker image if it doesn't exist
if [[ "$(docker images -q $IMAGE_NAME 2> /dev/null)" == "" ]]; then
  echo "Building Docker image: $IMAGE_NAME..."
  docker build -t $IMAGE_NAME .
fi

echo "Running build inside container..."

# Run the build inside the container
# Mount current directory to /workspace.
# Also mount a ccache volume for faster rebuilds.
mkdir -p .ccache
docker run --rm \
    -v "$(pwd):/workspace" \
    -v "$(pwd)/.ccache:/root/.ccache" \
    -e CCACHE_DIR=/root/.ccache \
    $IMAGE_NAME

echo "Build complete. Output: build/MugPlugin.ofx"
