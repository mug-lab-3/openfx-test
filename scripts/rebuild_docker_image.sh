#!/bin/bash
set -e

# Ensure we are in the repository root
cd "$(dirname "$0")/.."

IMAGE_NAME="resolve-ofx-win64-builder"

echo "=== Force rebuilding Docker image from scratch ==="

# 1. Remove existing image
if [[ "$(docker images -q $IMAGE_NAME 2> /dev/null)" != "" ]]; then
  echo "Removing existing image: $IMAGE_NAME..."
  docker rmi -f $IMAGE_NAME
fi

# 2. Prune build cache
echo "Pruning build cache..."
docker builder prune -f

# 3. Build image from scratch
echo "Building image without cache..."
docker build --no-cache -t $IMAGE_NAME -f docker/Dockerfile .

echo "=== Rebuild Complete! ==="
