SUMMARY = "RADAE Native Build - PyTorch Training Tools"
DESCRIPTION = "Native build of RADAE (Radio Autoencoder) for training and model development. \
Includes PyTorch, training scripts, and model export utilities."
HOMEPAGE = "https://github.com/drowe67/radae"
LICENSE = "BSD-2-Clause"
LIC_FILES_CHKSUM = "file://LICENSE;md5=d8bc78d05db4e904b2c78f400db5ec66"

DEPENDS = "python3-native"

# Use specific commit or tag for reproducible builds
SRCREV = "${AUTOREV}"
PV = "1.0+git${SRCPV}"

SRC_URI = "git://github.com/drowe67/radae.git;protocol=https;branch=main"

S = "${WORKDIR}/git"

inherit python3native

# Native build - runs on build host for training and development
inherit native

# Python dependencies for training
RDEPENDS:${PN} = " \
    python3-native \
    python3-numpy-native \
    python3-pytorch-native \
    python3-tqdm-native \
    python3-matplotlib-native \
"

do_configure() {
    # No configuration needed for Python scripts
    :
}

do_compile() {
    # No compilation needed for Python scripts
    :
}

do_install() {
    install -d ${D}${bindir}
    install -d ${D}${datadir}/radae
    install -d ${D}${datadir}/radae/radae
    install -d ${D}${datadir}/radae/weight-exchange

    # Install main training and inference scripts
    install -m 0755 ${S}/train.py ${D}${bindir}/radae-train
    install -m 0755 ${S}/inference.py ${D}${bindir}/radae-inference
    install -m 0755 ${S}/export_rade_weights.py ${D}${bindir}/radae-export-weights

    # Install training scripts for specific models
    install -m 0755 ${S}/train_bbfm.py ${D}${bindir}/radae-train-bbfm
    install -m 0755 ${S}/bbfm_inference.py ${D}${bindir}/radae-bbfm-inference

    # Install encoder/decoder tools
    install -m 0755 ${S}/stateful_encoder.py ${D}${bindir}/radae-stateful-encoder
    install -m 0755 ${S}/stateful_decoder.py ${D}${bindir}/radae-stateful-decoder

    # Install utility scripts
    install -m 0755 ${S}/est_snr.py ${D}${bindir}/radae-est-snr
    install -m 0755 ${S}/est_CNo.py ${D}${bindir}/radae-est-cno
    install -m 0755 ${S}/loss.py ${D}${bindir}/radae-loss
    install -m 0755 ${S}/chirp.py ${D}${bindir}/radae-chirp

    # Install conversion utilities
    install -m 0755 ${S}/f32toint16.py ${D}${bindir}/radae-f32toint16
    install -m 0755 ${S}/int16tof32.py ${D}${bindir}/radae-int16tof32

    # Install Python radae module
    install -m 0644 ${S}/radae/__init__.py ${D}${datadir}/radae/radae/
    install -m 0644 ${S}/radae/radae.py ${D}${datadir}/radae/radae/
    install -m 0644 ${S}/radae/radae_base.py ${D}${datadir}/radae/radae/
    install -m 0644 ${S}/radae/dataset.py ${D}${datadir}/radae/radae/
    install -m 0644 ${S}/radae/dsp.py ${D}${datadir}/radae/radae/
    install -m 0644 ${S}/radae/bbfm.py ${D}${datadir}/radae/radae/

    # Install weight-exchange module for model export
    install -m 0644 ${S}/weight-exchange/setup.py ${D}${datadir}/radae/weight-exchange/

    # Install helper scripts
    install -m 0755 ${S}/firtorch.py ${D}${datadir}/radae/
    install -m 0755 ${S}/ml_pilots.py ${D}${datadir}/radae/
}

FILES:${PN} = " \
    ${bindir}/* \
    ${datadir}/radae \
"

# Development package for training on build host
BBCLASSEXTEND = "native"
