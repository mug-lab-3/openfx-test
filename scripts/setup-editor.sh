#!/bin/bash
set -e

# Ensure we are in the repository root
cd "$(dirname "$0")/.."

IMAGE_NAME="resolve-ofx-win64-builder"
SDK_DIR=".sdk"

echo "=== Syncing SDK headers for Editor IntelliSense ==="

# Create local SDK directory
mkdir -p $SDK_DIR

# Run a temporary container to copy headers out
# We use a temp container to avoid permission issues
echo "Copying headers from container..."

docker run --rm -v "$(pwd)/$SDK_DIR:/mnt" $IMAGE_NAME bash -c "
    cp -r /opt/ofx-sdk /mnt/ && \
    cp -r /opt/3rdparty/blend2d /mnt/ && \
    cp -r /opt/3rdparty/asmjit /mnt/ && \
    chown -R $(id -u):$(id -g) /mnt/
"

echo "=== Sync Complete! ==="
echo "Headers are now available in: $(pwd)/$SDK_DIR"
echo ""
echo "Note: If you use VS Code, add these paths to your includePath."
echo "Or, your editor may now pick up the headers via compile_commands.json."
