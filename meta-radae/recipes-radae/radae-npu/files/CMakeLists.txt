cmake_minimum_required(VERSION 3.10)
project(radae-npu VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(ENABLE_NPU "Enable NPU acceleration" ON)
option(ENABLE_NEON "Enable NEON optimizations" ON)
option(ENABLE_OPENCV "Enable OpenCV for debugging" OFF)

# Find packages
find_package(PkgConfig REQUIRED)

# TensorFlow Lite
pkg_check_modules(TFLITE REQUIRED tensorflow-lite)

# VeriSilicon NPU delegate (optional)
if(ENABLE_NPU)
    pkg_check_modules(VSI_NPU vsi-npu)
    if(VSI_NPU_FOUND)
        add_definitions(-DENABLE_NPU)
        message(STATUS "VeriSilicon NPU delegate found")
    else()
        message(WARNING "VeriSilicon NPU delegate not found, NPU support disabled")
        set(ENABLE_NPU OFF)
    endif()
endif()

# OpenCV (optional)
if(ENABLE_OPENCV)
    find_package(OpenCV)
    if(OpenCV_FOUND)
        add_definitions(-DENABLE_OPENCV)
    endif()
endif()

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -ffast-math -DNDEBUG")

# NEON flags for ARM
if(ENABLE_NEON)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv8-a+simd -mtune=cortex-a53")
    add_definitions(-D__ARM_NEON)
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${TFLITE_INCLUDE_DIRS}
)

if(ENABLE_NPU AND VSI_NPU_FOUND)
    include_directories(${VSI_NPU_INCLUDE_DIRS})
endif()

# Source files
set(RADAE_NPU_SOURCES
    npu_encoder.cpp
    npu_decoder.cpp
    radae_streaming.cpp
)

# Create shared library
add_library(radae-npu SHARED ${RADAE_NPU_SOURCES})

# Link libraries
target_link_libraries(radae-npu
    ${TFLITE_LIBRARIES}
    dl
    pthread
)

if(ENABLE_NPU AND VSI_NPU_FOUND)
    target_link_libraries(radae-npu ${VSI_NPU_LIBRARIES})
endif()

if(ENABLE_OPENCV AND OpenCV_FOUND)
    target_link_libraries(radae-npu ${OpenCV_LIBS})
endif()

# Set library properties
set_target_properties(radae-npu PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER "radae_npu.h;npu_wrapper.h"
)

# Installation
install(TARGETS radae-npu
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/radae
)

# Print configuration summary
message(STATUS "")
message(STATUS "RADAE NPU Configuration:")
message(STATUS "  Version:           ${PROJECT_VERSION}")
message(STATUS "  NPU Support:       ${ENABLE_NPU}")
message(STATUS "  NEON Support:      ${ENABLE_NEON}")
message(STATUS "  OpenCV Support:    ${ENABLE_OPENCV}")
message(STATUS "  Build type:        ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Compiler:      ${CMAKE_CXX_COMPILER}")
message(STATUS "  C++ Flags:         ${CMAKE_CXX_FLAGS}")
message(STATUS "")
