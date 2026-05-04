#!/bin/bash
set -e

IMAGE_NAME="resolve-ofx-win64-builder"

# Build the image (generic)
echo "Ensuring Docker image is ready..."
docker build -t $IMAGE_NAME .

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

echo "Build complete. Output: build/MugPlugin.ofx"
