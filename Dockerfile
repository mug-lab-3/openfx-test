# Resolve OFX Win64 Builder
# Base on Ubuntu 24.04 (Noble) for modern LLVM/Clang
FROM ubuntu:24.04

# Avoid interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install core build tools
RUN sed -i 's|http://archive.ubuntu.com/ubuntu/|http://ftp.udx.icscoe.jp/Linux/ubuntu/|g' /etc/apt/sources.list.d/ubuntu.sources && \
    sed -i 's|http://security.ubuntu.com/ubuntu/|http://ftp.udx.icscoe.jp/Linux/ubuntu/|g' /etc/apt/sources.list.d/ubuntu.sources && \
    apt-get update && apt-get install -y \
    clang-18 \
    lld-18 \
    llvm-18 \
    mingw-w64 \
    cmake \
    ninja-build \
    make \
    ccache \
    git \
    ca-certificates \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Set Clang 18 and LLVM tools as default
RUN update-alternatives --install /usr/bin/clang clang /usr/bin/clang-18 100 && \
    update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang-18 100 && \
    update-alternatives --install /usr/bin/llvm-ar llvm-ar /usr/bin/llvm-ar-18 100 && \
    update-alternatives --install /usr/bin/llvm-ranlib llvm-ranlib /usr/bin/llvm-ranlib-18 100 && \
    update-alternatives --install /usr/bin/ld.lld ld.lld /usr/bin/lld-18 100

# Prepare SDK directories
WORKDIR /opt

# 1. OpenFX SDK
RUN git clone --depth 1 --branch OFX_Release_1.5.1 https://github.com/ofxa/openfx.git ofx-sdk

# 2. Blend2D & AsmJit
RUN mkdir -p 3rdparty && \
    git clone --depth 1 https://github.com/blend2d/blend2d.git 3rdparty/blend2d && \
    git clone --depth 1 https://github.com/asmjit/asmjit.git 3rdparty/asmjit

# Environment variables for CMake
ENV OFX_SDK_DIR=/opt/ofx-sdk
ENV BLEND2D_DIR=/opt/3rdparty/blend2d
ENV ASMJIT_DIR=/opt/3rdparty/asmjit

# Entrypoint setup
COPY entrypoint.sh /usr/local/bin/entrypoint.sh
RUN chmod +x /usr/local/bin/entrypoint.sh

# Workspace setup
WORKDIR /workspace
ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
