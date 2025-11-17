cmake_minimum_required(VERSION 3.10)
project(radae-npu-test VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(ENABLE_NPU "Enable NPU acceleration" ON)

# Find packages
find_package(PkgConfig REQUIRED)

# radae-npu library
pkg_check_modules(RADAE_NPU REQUIRED radae-npu)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${RADAE_NPU_INCLUDE_DIRS}
)

# Common test utilities
add_library(test-utils STATIC
    test_utils.cpp
)

target_link_libraries(test-utils
    ${RADAE_NPU_LIBRARIES}
)

# Benchmark executable
add_executable(radae-npu-benchmark
    npu_benchmark.cpp
)

target_link_libraries(radae-npu-benchmark
    test-utils
    ${RADAE_NPU_LIBRARIES}
    pthread
)

# Accuracy test executable
add_executable(radae-npu-accuracy
    npu_accuracy_test.cpp
)

target_link_libraries(radae-npu-accuracy
    test-utils
    ${RADAE_NPU_LIBRARIES}
    pthread
)

# Profiler executable
add_executable(radae-npu-profiler
    npu_profiler.cpp
)

target_link_libraries(radae-npu-profiler
    test-utils
    ${RADAE_NPU_LIBRARIES}
    pthread
)

# Installation
install(TARGETS
    radae-npu-benchmark
    radae-npu-accuracy
    radae-npu-profiler
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Print configuration
message(STATUS "")
message(STATUS "RADAE NPU Test Configuration:")
message(STATUS "  Version:           ${PROJECT_VERSION}")
message(STATUS "  NPU Support:       ${ENABLE_NPU}")
message(STATUS "  Build type:        ${CMAKE_BUILD_TYPE}")
message(STATUS "")
