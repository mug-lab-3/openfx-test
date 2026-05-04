#!/bin/bash
set -e

# デフォルトのビルドディレクトリ
BUILD_DIR="build"

# 引数がある場合は、そのコマンドを実行
if [ $# -gt 0 ]; then
    exec "$@"
fi

# 引数がない場合は、標準のビルドフローを実行
echo "--- Starting OpenFX Build (Windows x64) ---"

mkdir -p $BUILD_DIR
cmake -B $BUILD_DIR -GNinja -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain.cmake
cmake --build $BUILD_DIR

echo "--- Build Finished: $BUILD_DIR/MugPlugin.ofx ---"
