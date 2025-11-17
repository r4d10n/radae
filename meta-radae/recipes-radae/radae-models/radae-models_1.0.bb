SUMMARY = "RADAE Pre-trained Models"
DESCRIPTION = "Pre-trained PyTorch models for RADAE (Radio Autoencoder). Includes the \
production model19_check3 checkpoint and additional reference models for development."
HOMEPAGE = "https://github.com/drowe67/radae"
LICENSE = "BSD-2-Clause"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/BSD-2-Clause;md5=cb641bc04cda31daea161b1bc15da69f"

# Models are architecture-independent
PACKAGE_ARCH = "all"

# Fetch from the RADAE repository
SRCREV = "${AUTOREV}"
PV = "1.0+git${SRCPV}"

SRC_URI = " \
    git://github.com/drowe67/radae.git;protocol=https;branch=main \
"

S = "${WORKDIR}/git"

# No compilation needed
inherit allarch

do_configure[noexec] = "1"
do_compile[noexec] = "1"

do_install() {
    # Create model installation directory
    install -d ${D}${datadir}/radae/models

    # Install model19_check3 - Production model (primary model for runtime use)
    install -d ${D}${datadir}/radae/models/model19_check3/checkpoints
    if [ -f ${S}/model19_check3/checkpoints/checkpoint_epoch_100.pth ]; then
        install -m 0644 ${S}/model19_check3/checkpoints/checkpoint_epoch_100.pth \
            ${D}${datadir}/radae/models/model19_check3/checkpoints/
    else
        bbwarn "model19_check3 checkpoint not found - may need to download separately"
    fi

    # Install compiled model binaries for C implementation
    install -d ${D}${datadir}/radae/models/bin
    if [ -f ${S}/bin/model19_check3.bin ]; then
        install -m 0644 ${S}/bin/model19_check3.bin ${D}${datadir}/radae/models/bin/
    fi
    if [ -f ${S}/bin/model05.bin ]; then
        install -m 0644 ${S}/bin/model05.bin ${D}${datadir}/radae/models/bin/
    fi

    # Install reference models for development/testing
    for model in model05 model17 model18; do
        if [ -d ${S}/${model}/checkpoints ]; then
            install -d ${D}${datadir}/radae/models/${model}/checkpoints
            if [ -f ${S}/${model}/checkpoints/checkpoint_epoch_100.pth ]; then
                install -m 0644 ${S}/${model}/checkpoints/checkpoint_epoch_100.pth \
                    ${D}${datadir}/radae/models/${model}/checkpoints/
            fi
        fi
    done

    # Install BBFM model if present
    if [ -d ${S}/model_bbfm_01/checkpoints ]; then
        install -d ${D}${datadir}/radae/models/model_bbfm_01/checkpoints
        if [ -f ${S}/model_bbfm_01/checkpoints/checkpoint_epoch_100.pth ]; then
            install -m 0644 ${S}/model_bbfm_01/checkpoints/checkpoint_epoch_100.pth \
                ${D}${datadir}/radae/models/model_bbfm_01/checkpoints/
        fi
    fi

    # Create symlink for default model
    ln -sf model19_check3/checkpoints/checkpoint_epoch_100.pth \
        ${D}${datadir}/radae/models/default_model.pth

    # Install model metadata/README
    cat > ${D}${datadir}/radae/models/README << EOF
RADAE Pre-trained Models
========================

This directory contains pre-trained models for RADAE (Radio Autoencoder).

Primary Model:
- model19_check3: Production model for runtime use
  - Optimized for HF radio channels
  - 3000 Hz bandwidth, 4 dB SNR operating point
  - Includes auxdata for false sync detection

Reference Models:
- model05: Legacy reference model
- model17: Development reference
- model18: Alternative latent dimension (40)
- model_bbfm_01: Baseband FM model

Compiled Models (bin/):
- model19_check3.bin: Quantized weights for C implementation
- model05.bin: Reference model for C implementation

Default Model:
- default_model.pth -> model19_check3/checkpoints/checkpoint_epoch_100.pth

For more information: https://github.com/drowe67/radae
EOF
}

# Split into multiple packages
PACKAGES =+ "${PN}-production ${PN}-reference ${PN}-bbfm"

# Production model (minimal runtime footprint)
FILES:${PN}-production = " \
    ${datadir}/radae/models/model19_check3 \
    ${datadir}/radae/models/bin/model19_check3.bin \
    ${datadir}/radae/models/default_model.pth \
    ${datadir}/radae/models/README \
"

# Reference models for development
FILES:${PN}-reference = " \
    ${datadir}/radae/models/model05 \
    ${datadir}/radae/models/model17 \
    ${datadir}/radae/models/model18 \
    ${datadir}/radae/models/bin/model05.bin \
"

# BBFM model
FILES:${PN}-bbfm = " \
    ${datadir}/radae/models/model_bbfm_01 \
"

# Main package depends on production model
RDEPENDS:${PN} = "${PN}-production"
RRECOMMENDS:${PN} = "${PN}-reference"

# Runtime dependencies
RDEPENDS:${PN}-production = "python3-torch"
RDEPENDS:${PN}-reference = "python3-torch"
RDEPENDS:${PN}-bbfm = "python3-torch"

# Allow empty main package
ALLOW_EMPTY:${PN} = "1"

# Model files are data files
INSANE_SKIP:${PN}-production = "arch"
INSANE_SKIP:${PN}-reference = "arch"
INSANE_SKIP:${PN}-bbfm = "arch"
