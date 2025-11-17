SUMMARY = "RADAE NPU testing and benchmarking utilities"
DESCRIPTION = "Performance profiling, accuracy verification, and benchmark tools for NPU-accelerated RADAE"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

DEPENDS = "radae-npu tensorflow-lite"
RDEPENDS:${PN} = "radae-npu"

SRC_URI = " \
    file://npu_benchmark.cpp \
    file://npu_accuracy_test.cpp \
    file://npu_profiler.cpp \
    file://test_utils.h \
    file://test_data.h \
    file://CMakeLists.txt \
    file://test_vectors/speech_8k.raw \
    file://test_vectors/expected_features.bin \
"

S = "${WORKDIR}"

inherit cmake pkgconfig

EXTRA_OECMAKE = " \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_NPU=ON \
"

do_install() {
    install -d ${D}${bindir}
    install -d ${D}${datadir}/radae/test-vectors

    # Install test executables
    install -m 0755 ${B}/radae-npu-benchmark ${D}${bindir}/
    install -m 0755 ${B}/radae-npu-accuracy ${D}${bindir}/
    install -m 0755 ${B}/radae-npu-profiler ${D}${bindir}/

    # Install test data
    install -m 0644 ${S}/test_vectors/* ${D}${datadir}/radae/test-vectors/
}

# Create test scripts
do_install:append() {
    install -d ${D}${bindir}

    # Quick benchmark script
    cat > ${D}${bindir}/radae-npu-quick-test << 'EOF'
#!/bin/sh
# Quick NPU functionality test

echo "RADAE NPU Quick Test"
echo "===================="

# Check NPU availability
if [ -e /dev/galcore ]; then
    echo "✓ NPU device found"
else
    echo "✗ NPU device not found"
    exit 1
fi

# Run benchmark
echo ""
echo "Running performance benchmark..."
radae-npu-benchmark --quick

# Run accuracy test
echo ""
echo "Running accuracy verification..."
radae-npu-accuracy --test-vectors /usr/share/radae/test-vectors

echo ""
echo "Test complete!"
EOF
    chmod 0755 ${D}${bindir}/radae-npu-quick-test

    # Stress test script
    cat > ${D}${bindir}/radae-npu-stress-test << 'EOF'
#!/bin/sh
# NPU stress test

echo "RADAE NPU Stress Test"
echo "====================="

DURATION=${1:-300}  # Default 5 minutes

echo "Running stress test for ${DURATION} seconds..."
radae-npu-profiler --duration ${DURATION} --stress-mode

echo "Stress test complete!"
EOF
    chmod 0755 ${D}${bindir}/radae-npu-stress-test
}

FILES:${PN} = " \
    ${bindir}/radae-npu-benchmark \
    ${bindir}/radae-npu-accuracy \
    ${bindir}/radae-npu-profiler \
    ${bindir}/radae-npu-quick-test \
    ${bindir}/radae-npu-stress-test \
    ${datadir}/radae/test-vectors/* \
"

RDEPENDS:${PN} += "bash"
