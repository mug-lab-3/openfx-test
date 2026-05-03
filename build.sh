clang++ -target x86_64-pc-windows-gnu -shared -fPIC \
    -I./openfx-OFX_Release_1.5.1/include \
    -o MugPlugin.ofx \
    main.cpp \
    -fuse-ld=lld \
    -static \
    -Wl,--export-all-symbols
