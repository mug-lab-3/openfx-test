#!/bin/bash
clang++ -target x86_64-pc-windows-gnu -shared -fPIC \
    -I./openfx-OFX_Release_1.5.1/include \
    -I./openfx-OFX_Release_1.5.1/Support/include \
    -o MugPlugin.ofx \
    main.cpp \
    ./openfx-OFX_Release_1.5.1/Support/Library/ofxsCore.cpp \
    ./openfx-OFX_Release_1.5.1/Support/Library/ofxsImageEffect.cpp \
    ./openfx-OFX_Release_1.5.1/Support/Library/ofxsInteract.cpp \
    ./openfx-OFX_Release_1.5.1/Support/Library/ofxsLog.cpp \
    ./openfx-OFX_Release_1.5.1/Support/Library/ofxsMultiThread.cpp \
    ./openfx-OFX_Release_1.5.1/Support/Library/ofxsParams.cpp \
    ./openfx-OFX_Release_1.5.1/Support/Library/ofxsProperty.cpp \
    ./openfx-OFX_Release_1.5.1/Support/Library/ofxsPropertyValidation.cpp \
    -fuse-ld=lld \
    -static \
    -lopengl32 \
    -Wl,--export-all-symbols
