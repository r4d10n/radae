SUMMARY = "RADAE Example Configurations and Documentation"
DESCRIPTION = "Sample configuration files, test scripts, and documentation for RADAE"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

RDEPENDS:${PN} = "radae-app bash"

SRC_URI = " \
    file://radae-basic.conf \
    file://radae-low-latency.conf \
    file://radae-low-power.conf \
    file://radae-test.conf \
    file://test-audio-loopback.sh \
    file://test-npu-performance.sh \
    file://quick-start.sh \
    file://benchmark.sh \
    file://README.md \
    file://CONFIGURATION.md \
    file://TROUBLESHOOTING.md \
    file://generate-test-tone.sh \
"

S = "${WORKDIR}"

do_install() {
    # Install example configurations
    install -d ${D}${docdir}/radae/examples/configs
    install -m 0644 ${WORKDIR}/radae-basic.conf \
        ${D}${docdir}/radae/examples/configs/
    install -m 0644 ${WORKDIR}/radae-low-latency.conf \
        ${D}${docdir}/radae/examples/configs/
    install -m 0644 ${WORKDIR}/radae-low-power.conf \
        ${D}${docdir}/radae/examples/configs/
    install -m 0644 ${WORKDIR}/radae-test.conf \
        ${D}${docdir}/radae/examples/configs/

    # Install test scripts
    install -d ${D}${docdir}/radae/examples/scripts
    install -m 0755 ${WORKDIR}/test-audio-loopback.sh \
        ${D}${docdir}/radae/examples/scripts/
    install -m 0755 ${WORKDIR}/test-npu-performance.sh \
        ${D}${docdir}/radae/examples/scripts/
    install -m 0755 ${WORKDIR}/quick-start.sh \
        ${D}${docdir}/radae/examples/scripts/
    install -m 0755 ${WORKDIR}/benchmark.sh \
        ${D}${docdir}/radae/examples/scripts/
    install -m 0755 ${WORKDIR}/generate-test-tone.sh \
        ${D}${docdir}/radae/examples/scripts/

    # Install documentation
    install -d ${D}${docdir}/radae
    install -m 0644 ${WORKDIR}/README.md ${D}${docdir}/radae/
    install -m 0644 ${WORKDIR}/CONFIGURATION.md ${D}${docdir}/radae/
    install -m 0644 ${WORKDIR}/TROUBLESHOOTING.md ${D}${docdir}/radae/

    # Create test data directory
    install -d ${D}${datadir}/radae/test-data
}

FILES:${PN} = " \
    ${docdir}/radae \
    ${datadir}/radae/test-data \
"

FILES:${PN}-doc = " \
    ${docdir}/radae/*.md \
    ${docdir}/radae/examples \
"

pkg_postinst:${PN}() {
    if [ -z "$D" ]; then
        echo ""
        echo "RADAE Examples installed!"
        echo ""
        echo "Documentation: ${docdir}/radae/"
        echo "Example configs: ${docdir}/radae/examples/configs/"
        echo "Test scripts: ${docdir}/radae/examples/scripts/"
        echo ""
        echo "Quick start: ${docdir}/radae/examples/scripts/quick-start.sh"
        echo ""
    fi
}
