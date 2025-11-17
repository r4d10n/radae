SUMMARY = "RADAE Full-Duplex HF Voice Application"
DESCRIPTION = "Complete RADAE TX/RX application with ALSA audio, OFDM modem, and NPU acceleration"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

DEPENDS = "alsa-lib radae-npu libconfig libsamplerate"
RDEPENDS:${PN} = " \
    alsa-lib \
    radae-npu \
    libconfig \
    libsamplerate \
    bash \
"

SRC_URI = " \
    file://radae_main.c \
    file://audio_capture.c \
    file://audio_capture.h \
    file://ofdm_neon.c \
    file://ofdm_neon.h \
    file://fft_neon.c \
    file://fft_neon.h \
    file://config_parser.c \
    file://config_parser.h \
    file://ptt_control.c \
    file://ptt_control.h \
    file://CMakeLists.txt \
    file://radae.1 \
"

S = "${WORKDIR}"

inherit cmake pkgconfig

EXTRA_OECMAKE = " \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_NEON=ON \
    -DENABLE_NPU=ON \
    -DUSE_ALSA=ON \
"

# ARM NEON optimization flags
CFLAGS:append = " -march=armv8-a+simd -mtune=cortex-a53 -O3 -ffast-math"

# Package configuration options
PACKAGECONFIG ??= "gpio ptt"
PACKAGECONFIG[gpio] = "-DENABLE_GPIO=ON,-DENABLE_GPIO=OFF,libgpiod"
PACKAGECONFIG[ptt] = "-DENABLE_PTT=ON,-DENABLE_PTT=OFF"
PACKAGECONFIG[debug] = "-DENABLE_DEBUG=ON,-DENABLE_DEBUG=OFF"

do_install() {
    install -d ${D}${bindir}
    install -d ${D}${sysconfdir}/radae
    install -d ${D}${datadir}/radae
    install -d ${D}${mandir}/man1

    # Install binaries
    install -m 0755 ${B}/radae-tx ${D}${bindir}/
    install -m 0755 ${B}/radae-rx ${D}${bindir}/
    install -m 0755 ${B}/radae-trx ${D}${bindir}/

    # Install default configuration
    cat > ${D}${sysconfdir}/radae/radae.conf << 'EOF'
# RADAE Application Configuration
# Audio Configuration
[audio]
sample_rate = 16000
channels = 1
period_size = 160
buffer_size = 1600
capture_device = "hw:0,0"
playback_device = "hw:0,0"

# OFDM Modem Configuration
[ofdm]
sample_rate = 8000
num_carriers = 30
symbol_rate = 50
cp_length = 0.002
pilot_spacing = 4

# NPU Configuration
[npu]
enable = 1
encoder_model = "/usr/share/radae/radae_encoder_int8.tflite"
decoder_model = "/usr/share/radae/radae_decoder_int8.tflite"
num_threads = 2

# PTT Configuration
[ptt]
enable = 0
gpio_chip = "gpiochip0"
gpio_line = 12
active_low = 0
delay_ms = 100

# Logging
[logging]
level = "info"
file = "/var/log/radae/radae.log"
EOF

    # Install man page
    install -m 0644 ${WORKDIR}/radae.1 ${D}${mandir}/man1/
}

do_install:append() {
    # Create log directory
    install -d ${D}${localstatedir}/log/radae
}

FILES:${PN} = " \
    ${bindir}/radae-tx \
    ${bindir}/radae-rx \
    ${bindir}/radae-trx \
    ${sysconfdir}/radae/radae.conf \
    ${datadir}/radae \
    ${localstatedir}/log/radae \
"

FILES:${PN}-doc = " \
    ${mandir}/man1/radae.1 \
"

# Allow runtime configuration override
CONFFILES:${PN} = "${sysconfdir}/radae/radae.conf"

# Ensure proper permissions for audio and GPIO
pkg_postinst:${PN}() {
    if [ -z "$D" ]; then
        # Create radae user/group if not exists
        if ! grep -q radae /etc/group; then
            groupadd -r radae
        fi
        if ! id radae > /dev/null 2>&1; then
            useradd -r -g radae -G audio,gpio -d /var/lib/radae -s /bin/false radae
        fi

        # Set log directory permissions
        chown radae:radae /var/log/radae
        chmod 755 /var/log/radae
    fi
}
