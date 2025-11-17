SUMMARY = "RADAE - Radio Autoencoder"
DESCRIPTION = "Radio Autoencoder (RADAE) is a Machine Learning model that encodes speech \
features for robust transmission over radio channels. This package contains the optimized \
C implementation with NEON/SIMD support and FARGAN vocoder integration."
HOMEPAGE = "https://github.com/drowe67/radae"
LICENSE = "BSD-2-Clause"
LIC_FILES_CHKSUM = "file://LICENSE;md5=d8bc78d05db4e904b2c78f400db5ec66"

DEPENDS = " \
    python3 \
    python3-numpy \
    python3-numpy-native \
    opus \
"

# Use specific commit or tag for reproducible builds
SRCREV = "${AUTOREV}"
PV = "1.0+git${SRCPV}"

SRC_URI = " \
    git://github.com/drowe67/radae.git;protocol=https;branch=main \
    file://0001-cmake-fix-cross-compile.patch;apply=no \
"

S = "${WORKDIR}/git"

inherit cmake pkgconfig python3native

# Configuration options
PACKAGECONFIG ??= "${@bb.utils.filter('TUNE_FEATURES', 'neon', d)}"
PACKAGECONFIG[neon] = "-DAVX=OFF,-DAVX=OFF"
PACKAGECONFIG[avx] = "-DAVX=ON,-DAVX=OFF"

# Extra CMake arguments
EXTRA_OECMAKE = " \
    -DBUILD_SHARED_LIBS=ON \
    -DCMAKE_BUILD_TYPE=Release \
    -DPython3_ROOT_DIR=${STAGING_DIR_TARGET}${prefix} \
    -DPython3_FIND_REGISTRY=NEVER \
    -DPython3_NumPy_INCLUDE_DIR=${STAGING_DIR_TARGET}${PYTHON_SITEPACKAGES_DIR}/numpy/_core/include \
"

# ARM NEON optimizations
CFLAGS:append:armv7a = " -mfpu=neon"
CFLAGS:append:aarch64 = " -march=armv8-a"

# Set Python paths for cross-compilation
export PYTHONPATH = "${STAGING_DIR_TARGET}${PYTHON_SITEPACKAGES_DIR}"

do_configure:prepend() {
    # For cross-compilation, we need to tell CMake about Python location
    if [ "${@oe.utils.conditional('CLASSOVERRIDE', 'class-target', 'true', 'false', d)}" = "true" ]; then
        bbnote "Configuring for cross-compilation with Python at ${STAGING_DIR_TARGET}${prefix}"
    fi
}

do_compile() {
    cmake --build ${B} -- -j${@oe.utils.cpu_count()}
}

do_install() {
    # Install libraries
    install -d ${D}${libdir}
    install -m 0755 ${B}/src/librade.so* ${D}${libdir}/

    # Install binaries
    install -d ${D}${bindir}
    install -m 0755 ${B}/src/lpcnet_demo ${D}${bindir}/
    install -m 0755 ${B}/src/radae_tx ${D}${bindir}/
    install -m 0755 ${B}/src/radae_rx ${D}${bindir}/

    if [ -f ${B}/src/test_rade_enc ]; then
        install -m 0755 ${B}/src/test_rade_enc ${D}${bindir}/
        install -m 0755 ${B}/src/test_rade_dec ${D}${bindir}/
    fi

    # Install headers
    install -d ${D}${includedir}/radae
    install -m 0644 ${S}/src/rade_api.h ${D}${includedir}/radae/
    install -m 0644 ${S}/src/rade_core.h ${D}${includedir}/radae/
    install -m 0644 ${S}/src/rade_constants.h ${D}${includedir}/radae/
    install -m 0644 ${S}/src/rade_enc.h ${D}${includedir}/radae/
    install -m 0644 ${S}/src/rade_dec.h ${D}${includedir}/radae/

    # Install Python modules for embedded use
    install -d ${D}${PYTHON_SITEPACKAGES_DIR}/radae
    install -m 0644 ${S}/radae/__init__.py ${D}${PYTHON_SITEPACKAGES_DIR}/radae/
    install -m 0644 ${S}/radae/radae.py ${D}${PYTHON_SITEPACKAGES_DIR}/radae/
    install -m 0644 ${S}/radae/radae_base.py ${D}${PYTHON_SITEPACKAGES_DIR}/radae/
    install -m 0644 ${S}/radae/dsp.py ${D}${PYTHON_SITEPACKAGES_DIR}/radae/

    # Install runtime Python scripts
    install -m 0755 ${S}/radae_txe.py ${D}${bindir}/
    install -m 0755 ${S}/radae_rxe.py ${D}${bindir}/
    install -m 0755 ${S}/rx.py ${D}${bindir}/

    # Install pkg-config file
    install -d ${D}${libdir}/pkgconfig
    cat > ${D}${libdir}/pkgconfig/radae.pc << EOF
prefix=${prefix}
exec_prefix=${exec_prefix}
libdir=${libdir}
includedir=${includedir}

Name: RADAE
Description: Radio Autoencoder Library
Version: ${PV}
Requires: python3
Libs: -L\${libdir} -lrade -lopus -lm
Cflags: -I\${includedir}/radae
EOF
}

# Runtime dependencies
RDEPENDS:${PN} = " \
    python3-core \
    python3-numpy \
    python3-torch \
    opus \
"

# Development package
PACKAGES =+ "${PN}-dev ${PN}-dbg"

FILES:${PN} = " \
    ${libdir}/librade.so.* \
    ${bindir}/lpcnet_demo \
    ${bindir}/radae_tx \
    ${bindir}/radae_rx \
    ${bindir}/radae_txe.py \
    ${bindir}/radae_rxe.py \
    ${bindir}/rx.py \
    ${PYTHON_SITEPACKAGES_DIR}/radae \
"

FILES:${PN}-dev = " \
    ${libdir}/librade.so \
    ${includedir}/radae \
    ${libdir}/pkgconfig/radae.pc \
    ${bindir}/test_rade_enc \
    ${bindir}/test_rade_dec \
"

FILES:${PN}-dbg = " \
    ${bindir}/.debug \
    ${libdir}/.debug \
"

# Library provides
PROVIDES = "radae"

# Shared library versioning
SOLIBS = ".so.0.1"
SOLIBSDEV = ".so"

# Allow commercial license for dependencies
LICENSE_FLAGS = "commercial"
