cmake_minimum_required(VERSION 3.10)
project(radae-app C)

set(CMAKE_C_STANDARD 11)

# Build options
option(ENABLE_NPU "Enable NPU acceleration" ON)
option(ENABLE_NEON "Enable ARM NEON optimizations" ON)
option(ENABLE_GPIO "Enable GPIO support for PTT" ON)
option(USE_ALSA "Use ALSA for audio" ON)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_C_FLAGS_RELEASE "-O3 -ffast-math")

if(ENABLE_NEON)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv8-a+simd -mtune=cortex-a53")
    add_definitions(-D__ARM_NEON)
endif()

if(ENABLE_NPU)
    add_definitions(-DENABLE_NPU)
endif()

if(ENABLE_GPIO)
    add_definitions(-DENABLE_GPIO)
endif()

# Find dependencies
find_package(PkgConfig REQUIRED)

if(USE_ALSA)
    pkg_check_modules(ALSA REQUIRED alsa)
endif()

if(ENABLE_GPIO)
    pkg_check_modules(GPIOD REQUIRED libgpiod)
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

if(ENABLE_NPU)
    include_directories(/usr/include/radae)
endif()

# Common sources
set(COMMON_SOURCES
    audio_capture.c
    ofdm_neon.c
    fft_neon.c
    config_parser.c
    ptt_control.c
)

# radae-trx: Full-duplex application
add_executable(radae-trx
    radae_main.c
    ${COMMON_SOURCES}
)

target_link_libraries(radae-trx
    ${ALSA_LIBRARIES}
    m
    pthread
)

if(ENABLE_NPU)
    target_link_libraries(radae-trx radae-npu)
endif()

if(ENABLE_GPIO)
    target_link_libraries(radae-trx ${GPIOD_LIBRARIES})
endif()

# radae-tx: TX-only application
add_executable(radae-tx radae_main.c ${COMMON_SOURCES})
target_compile_definitions(radae-tx PRIVATE TX_ONLY)
target_link_libraries(radae-tx ${ALSA_LIBRARIES} m pthread)

if(ENABLE_NPU)
    target_link_libraries(radae-tx radae-npu)
endif()

if(ENABLE_GPIO)
    target_link_libraries(radae-tx ${GPIOD_LIBRARIES})
endif()

# radae-rx: RX-only application
add_executable(radae-rx radae_main.c ${COMMON_SOURCES})
target_compile_definitions(radae-rx PRIVATE RX_ONLY)
target_link_libraries(radae-rx ${ALSA_LIBRARIES} m pthread)

if(ENABLE_NPU)
    target_link_libraries(radae-rx radae-npu)
endif()

# Installation
install(TARGETS radae-trx radae-tx radae-rx
    RUNTIME DESTINATION bin
)
