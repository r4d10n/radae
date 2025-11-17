SUMMARY = "RADAE Development and Test Tools"
DESCRIPTION = "Development, testing, and evaluation tools for RADAE (Radio Autoencoder). \
Includes inference scripts, test utilities, evaluation tools, and shell scripts for \
testing various channel conditions and modem configurations."
HOMEPAGE = "https://github.com/drowe67/radae"
LICENSE = "BSD-2-Clause"
LIC_FILES_CHKSUM = "file://LICENSE;md5=d8bc78d05db4e904b2c78f400db5ec66"

DEPENDS = "radae radae-models"

# Fetch from the RADAE repository
SRCREV = "${AUTOREV}"
PV = "1.0+git${SRCPV}"

SRC_URI = " \
    git://github.com/drowe67/radae.git;protocol=https;branch=main \
"

S = "${WORKDIR}/git"

inherit python3-dir

do_configure[noexec] = "1"
do_compile[noexec] = "1"

do_install() {
    # Create installation directories
    install -d ${D}${bindir}
    install -d ${D}${datadir}/radae/tools
    install -d ${D}${datadir}/radae/test
    install -d ${D}${datadir}/radae/wav
    install -d ${D}${datadir}/radae/doc

    # Install inference and testing scripts
    install -m 0755 ${S}/inference.sh ${D}${bindir}/radae-inference
    install -m 0755 ${S}/rx.sh ${D}${bindir}/radae-rx
    install -m 0755 ${S}/radae_rx.sh ${D}${bindir}/radae-rx-stream

    # Install evaluation tools
    install -m 0755 ${S}/evaluate.sh ${D}${bindir}/radae-evaluate
    install -m 0755 ${S}/evaluate_loop.sh ${D}${bindir}/radae-evaluate-loop
    install -m 0755 ${S}/compare_models.sh ${D}${bindir}/radae-compare-models
    install -m 0755 ${S}/est_snr_curves.sh ${D}${bindir}/radae-est-snr-curves

    # Install OTA (over-the-air) test tools
    install -m 0755 ${S}/ota_test.sh ${D}${bindir}/radae-ota-test
    install -m 0755 ${S}/ptt_test.sh ${D}${bindir}/radae-ptt-test
    install -m 0755 ${S}/ofdm_sync.sh ${D}${bindir}/radae-ofdm-sync

    # Install BBFM tools
    install -m 0755 ${S}/bbfm_inference.sh ${D}${bindir}/radae-bbfm-inference
    install -m 0755 ${S}/bbfm_rx.sh ${D}${bindir}/radae-bbfm-rx
    install -m 0755 ${S}/analog_bbfm.sh ${D}${bindir}/radae-analog-bbfm

    # Install stateful encoder/decoder test scripts
    install -m 0755 ${S}/stateful_encoder.sh ${D}${bindir}/radae-test-stateful-encoder
    install -m 0755 ${S}/stateful_decoder.sh ${D}${bindir}/radae-test-stateful-decoder

    # Install demonstration scripts
    install -m 0755 ${S}/rick.sh ${D}${bindir}/radae-demo-rick
    install -m 0755 ${S}/dt_test.sh ${D}${bindir}/radae-test-dt

    # Install Python analysis tools
    install -m 0755 ${S}/inference.py ${D}${datadir}/radae/tools/
    install -m 0755 ${S}/rx.py ${D}${datadir}/radae/tools/
    install -m 0755 ${S}/loss.py ${D}${datadir}/radae/tools/
    install -m 0755 ${S}/est_snr.py ${D}${datadir}/radae/tools/
    install -m 0755 ${S}/est_CNo.py ${D}${datadir}/radae/tools/
    install -m 0755 ${S}/eoo_ber.py ${D}${datadir}/radae/tools/

    # Install single carrier modem tools
    install -m 0755 ${S}/sc_tx.py ${D}${datadir}/radae/tools/
    install -m 0755 ${S}/sc_rx.py ${D}${datadir}/radae/tools/

    # Install utility scripts
    install -m 0755 ${S}/utils.sh ${D}${datadir}/radae/tools/

    # Install test suite
    install -d ${D}${datadir}/radae/test
    if [ -d ${S}/test ]; then
        for testfile in ${S}/test/*.sh; do
            if [ -f "$testfile" ]; then
                install -m 0755 "$testfile" ${D}${datadir}/radae/test/
            fi
        done
    fi

    # Install sample WAV files for testing
    if [ -d ${S}/wav ]; then
        for wavfile in ${S}/wav/*.wav; do
            if [ -f "$wavfile" ]; then
                install -m 0644 "$wavfile" ${D}${datadir}/radae/wav/
            fi
        done
    fi

    # Install MATLAB/Octave analysis scripts
    install -m 0644 ${S}/radae_plots.m ${D}${datadir}/radae/tools/
    install -m 0644 ${S}/fm.m ${D}${datadir}/radae/tools/
    install -m 0644 ${S}/bbfm_bpf.m ${D}${datadir}/radae/tools/
    install -m 0644 ${S}/load_f32.m ${D}${datadir}/radae/tools/
    install -m 0644 ${S}/load_raw.m ${D}${datadir}/radae/tools/
    install -m 0644 ${S}/plot_specgram.m ${D}${datadir}/radae/tools/
    install -m 0644 ${S}/multipath_samples.m ${D}${datadir}/radae/tools/
    install -m 0644 ${S}/doppler_spread.m ${D}${datadir}/radae/tools/

    # Install documentation
    install -m 0644 ${S}/README.md ${D}${datadir}/radae/doc/
    install -m 0644 ${S}/BBFM.md ${D}${datadir}/radae/doc/

    # Install configuration files
    install -m 0644 ${S}/ptt_test.conf ${D}${datadir}/radae/tools/

    # Create wrapper scripts that use installed models
    cat > ${D}${bindir}/radae-test-model19 << 'EOF'
#!/bin/sh
# Test script for model19_check3
MODEL_PATH="${RADAE_MODEL_PATH:-/usr/share/radae/models/model19_check3/checkpoints/checkpoint_epoch_100.pth}"
INPUT_WAV="${1:-/usr/share/radae/wav/brian_g8sez.wav}"
OUTPUT_WAV="${2:-/tmp/output.wav}"
EBNODB="${3:-3}"

exec /usr/share/radae/tools/inference.py \
    --model "${MODEL_PATH}" \
    --input "${INPUT_WAV}" \
    --output "${OUTPUT_WAV}" \
    --rate_Fs --pilots --pilot_eq --eq_ls --cp 0.004 --bottleneck 3 \
    --EbNodB "${EBNODB}" "$@"
EOF
    chmod 0755 ${D}${bindir}/radae-test-model19

    # Create quick test script
    cat > ${D}${bindir}/radae-quick-test << 'EOF'
#!/bin/sh
# Quick sanity test of RADAE installation
echo "RADAE Quick Test"
echo "================"
echo ""
echo "Checking installation..."

# Check binaries
for bin in radae_tx radae_rx lpcnet_demo; do
    if ! command -v $bin >/dev/null 2>&1; then
        echo "ERROR: $bin not found"
        exit 1
    fi
done
echo "✓ Binaries installed"

# Check models
MODEL_DIR="/usr/share/radae/models"
if [ ! -f "${MODEL_DIR}/model19_check3/checkpoints/checkpoint_epoch_100.pth" ]; then
    echo "ERROR: Production model not found"
    exit 1
fi
echo "✓ Models installed"

# Check Python modules
python3 -c "import radae" 2>/dev/null
if [ $? -ne 0 ]; then
    echo "ERROR: Python radae module not importable"
    exit 1
fi
echo "✓ Python modules installed"

echo ""
echo "Installation appears healthy!"
echo ""
echo "Try running: radae-test-model19 <input.wav> <output.wav>"
EOF
    chmod 0755 ${D}${bindir}/radae-quick-test
}

# Runtime dependencies
RDEPENDS:${PN} = " \
    radae \
    radae-models \
    python3-core \
    python3-numpy \
    python3-torch \
    python3-matplotlib \
    bash \
"

# Optional runtime recommendations
RRECOMMENDS:${PN} = " \
    codec2 \
    sox \
"

FILES:${PN} = " \
    ${bindir}/radae-* \
    ${datadir}/radae/tools \
    ${datadir}/radae/test \
    ${datadir}/radae/wav \
    ${datadir}/radae/doc \
"

# These are test/development tools
ALLOW_EMPTY:${PN} = "0"
