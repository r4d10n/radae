SUMMARY = "RADAE NPU-accelerated implementation for i.MX 8M Plus"
DESCRIPTION = "Neural vocoder with NPU acceleration using VeriSilicon delegate"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

DEPENDS = "tensorflow-lite imx-vsi-npu"
RDEPENDS:${PN} = "tensorflow-lite imx-vsi-npu"

SRC_URI = " \
    file://npu_encoder.cpp \
    file://npu_decoder.cpp \
    file://radae_streaming.cpp \
    file://npu_wrapper.h \
    file://radae_npu.h \
    file://CMakeLists.txt \
"

S = "${WORKDIR}"

inherit cmake pkgconfig

EXTRA_OECMAKE = " \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_NPU=ON \
    -DENABLE_NEON=ON \
    -DTFLITE_ENABLE_GPU=OFF \
"

# NPU-specific compiler flags
CXXFLAGS:append = " -march=armv8-a+simd -mtune=cortex-a53 -O3 -ffast-math"

# Check for NPU availability at runtime
PACKAGECONFIG ??= "${@bb.utils.contains('MACHINE_FEATURES', 'npu', 'npu', '', d)}"
PACKAGECONFIG[npu] = "-DENABLE_NPU=ON,-DENABLE_NPU=OFF,imx-vsi-npu"
PACKAGECONFIG[opencv] = "-DENABLE_OPENCV=ON,-DENABLE_OPENCV=OFF,opencv"

do_configure:prepend() {
    # Verify NPU delegate library exists
    if [ "${@bb.utils.contains('PACKAGECONFIG', 'npu', 'yes', 'no', d)}" = "yes" ]; then
        if ! pkg-config --exists vsi-npu; then
            bbwarn "VeriSilicon NPU delegate not found, falling back to CPU"
        fi
    fi
}

do_install() {
    install -d ${D}${libdir}
    install -d ${D}${includedir}/radae

    # Install shared library
    install -m 0755 ${B}/libradae-npu.so ${D}${libdir}/

    # Install headers
    install -m 0644 ${S}/radae_npu.h ${D}${includedir}/radae/
    install -m 0644 ${S}/npu_wrapper.h ${D}${includedir}/radae/
}

# Create runtime configuration directory
do_install:append() {
    install -d ${D}${sysconfdir}/radae

    # Create default configuration
    cat > ${D}${sysconfdir}/radae/npu.conf << EOF
# RADAE NPU Configuration
ENABLE_NPU=1
QUANTIZATION_LEVEL=INT8
FALLBACK_TO_CPU=1
NPU_CACHE_SIZE=4096
THREAD_COUNT=4
EOF
}

FILES:${PN} = " \
    ${libdir}/libradae-npu.so \
    ${sysconfdir}/radae/npu.conf \
"

FILES:${PN}-dev = " \
    ${includedir}/radae/*.h \
"

INSANE_SKIP:${PN} = "ldflags"

# Ensure NPU firmware is available
RRECOMMENDS:${PN} += "imx-vpu-hantro firmware-imx-vpu-imx8"
