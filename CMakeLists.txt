cmake_minimum_required(VERSION 3.10)
project(MugPlugin VERSION 1.0.0 LANGUAGES CXX)

# Enable ccache if available
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# OFX SDK Paths
set(OFX_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/openfx-OFX_Release_1.5.1")
set(OFX_INCLUDE_DIRS
    "${OFX_SDK_DIR}/include"
    "${OFX_SDK_DIR}/Support/include"
)

# Support Library Sources
set(OFX_SUPPORT_SOURCES
    "${OFX_SDK_DIR}/Support/Library/ofxsCore.cpp"
    "${OFX_SDK_DIR}/Support/Library/ofxsImageEffect.cpp"
    "${OFX_SDK_DIR}/Support/Library/ofxsInteract.cpp"
    "${OFX_SDK_DIR}/Support/Library/ofxsLog.cpp"
    "${OFX_SDK_DIR}/Support/Library/ofxsMultiThread.cpp"
    "${OFX_SDK_DIR}/Support/Library/ofxsParams.cpp"
    "${OFX_SDK_DIR}/Support/Library/ofxsProperty.cpp"
    "${OFX_SDK_DIR}/Support/Library/ofxsPropertyValidation.cpp"
)

# Define the Plugin Target
add_library(MugPlugin SHARED
    main.cpp
    ${OFX_SUPPORT_SOURCES}
)

# Include Directories
target_include_directories(MugPlugin PRIVATE ${OFX_INCLUDE_DIRS})

# Position Independent Code
set_target_properties(MugPlugin PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Handle Windows / Cross-compilation settings
if(WIN32 OR MINGW OR CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_libraries(MugPlugin PRIVATE opengl32)
    target_link_options(MugPlugin PRIVATE 
        "-fuse-ld=lld"
        "-static"
        "-Wl,--export-all-symbols"
    )
endif()

# Set output extension to .ofx
set_target_properties(MugPlugin PROPERTIES 
    PREFIX "" 
    SUFFIX ".ofx"
)
