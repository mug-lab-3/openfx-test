cmake_minimum_required(VERSION 3.10)
project(MugPlugin VERSION 1.0.0 LANGUAGES CXX)

# Enable ccache if available
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

set(CMAKE_CXX_STANDARD 20)
# --- SDK & Dependency Resolution ---

# 1. AsmJit
if(NOT DEFINED ASMJIT_DIR)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/asmjit")
        set(ASMJIT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/asmjit" CACHE PATH "")
    elseif(EXISTS "/opt/3rdparty/asmjit")
        set(ASMJIT_DIR "/opt/3rdparty/asmjit" CACHE PATH "")
    endif()
endif()

# 2. Blend2D
set(BLEND2D_TEST OFF CACHE BOOL "" FORCE)
set(BLEND2D_SAMPLES OFF CACHE BOOL "" FORCE)
set(BLEND2D_STATIC ON CACHE BOOL "" FORCE)
set(BLEND2D_NO_STDCXX OFF CACHE BOOL "" FORCE)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/blend2d")
    add_subdirectory(3rdparty/blend2d)
elseif(EXISTS "/opt/3rdparty/blend2d")
    add_subdirectory("/opt/3rdparty/blend2d" "${CMAKE_CURRENT_BINARY_DIR}/blend2d")
endif()

# 3. OpenFX SDK
if(NOT DEFINED OFX_SDK_DIR)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/openfx-OFX_Release_1.5.1")
        set(OFX_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/openfx-OFX_Release_1.5.1")
    elseif(EXISTS "/opt/ofx-sdk")
        set(OFX_SDK_DIR "/opt/ofx-sdk")
    endif()
endif()

# 4. Resolve Blend2D Source Path for Includes
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/blend2d/blend2d")
    set(BLEND2D_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/blend2d")
elseif(EXISTS "/opt/3rdparty/blend2d/blend2d")
    set(BLEND2D_SRC_DIR "/opt/3rdparty/blend2d")
endif()

# --- Build Options ---
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
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
    main.cc
    ${OFX_SUPPORT_SOURCES}
)

# Include Directories
target_include_directories(MugPlugin PRIVATE 
    ${OFX_INCLUDE_DIRS} 
    ${BLEND2D_SRC_DIR}
    ${BLEND2D_SRC_DIR}/blend2d
)

# Link Libraries
target_link_libraries(MugPlugin PRIVATE blend2d)

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
