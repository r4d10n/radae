# recipes-core

This directory contains BitBake recipes for core system modifications.

## Purpose

Houses recipes that modify core system components:

- **images**: Custom image recipes for RADAE
- **systemd**: Systemd service files and configurations
- **init-scripts**: Initialization and startup scripts
- **base-files**: System base file modifications
- **packagegroups**: Package groups for RADAE

## Image Recipes

### Custom RADAE Image

```
recipes-core/images/
├── radae-image-minimal.bb
├── radae-image-base.bb
└── radae-image-full.bb
```

### Example: radae-image-base.bb

```bitbake
SUMMARY = "RADAE Base Image for i.MX 8M Plus"
LICENSE = "MIT"

inherit core-image

# Base packages
IMAGE_INSTALL = " \
    packagegroup-core-boot \
    packagegroup-core-full-cmdline \
    ${CORE_IMAGE_EXTRA_INSTALL} \
"

# Machine learning packages
IMAGE_INSTALL += " \
    tensorflow-lite \
    python3-tflite-runtime \
    imx-npu \
    vx-delegate \
"

# RADAE packages
IMAGE_INSTALL += " \
    radae-core \
    radae-encoder \
    radae-decoder \
    radae-models \
    radae-tools \
"

# Audio support
IMAGE_INSTALL += " \
    alsa-utils \
    alsa-lib \
    alsa-plugins \
"

# System utilities
IMAGE_INSTALL += " \
    systemd \
    systemd-analyze \
    htop \
    vim \
    tmux \
    gdb \
    strace \
"

# Network tools
IMAGE_INSTALL += " \
    openssh \
    iproute2 \
    iputils \
    ethtool \
"

# Development tools (optional)
IMAGE_INSTALL += " \
    python3-dev \
    python3-pip \
    gcc \
    make \
    cmake \
    git \
"

# Image features
IMAGE_FEATURES += " \
    ssh-server-openssh \
    package-management \
    debug-tweaks \
    tools-debug \
    tools-profile \
"

# Root filesystem size
IMAGE_ROOTFS_SIZE ?= "8192"
IMAGE_ROOTFS_EXTRA_SPACE = "2097152"

# Image format
IMAGE_FSTYPES = "tar.bz2 wic.bz2 wic.bmap"

# Post-install commands
ROOTFS_POSTPROCESS_COMMAND += "radae_postinstall; "

radae_postinstall() {
    # Create RADAE directories
    install -d ${IMAGE_ROOTFS}${RADAE_MODEL_DIR}
    install -d ${IMAGE_ROOTFS}${RADAE_CONFIG_DIR}
    install -d ${IMAGE_ROOTFS}${RADAE_DATA_DIR}

    # Set permissions
    chmod 755 ${IMAGE_ROOTFS}${RADAE_MODEL_DIR}
    chmod 755 ${IMAGE_ROOTFS}${RADAE_CONFIG_DIR}
    chmod 755 ${IMAGE_ROOTFS}${RADAE_DATA_DIR}
}
```

## Systemd Services

### Service File Recipe

```
recipes-core/systemd/
├── radae-service.bb
└── radae-service/
    ├── radae-encoder.service
    ├── radae-decoder.service
    └── radae-monitor.service
```

### Example: radae-encoder.service

```systemd
[Unit]
Description=RADAE Encoder Service
After=network.target sound.target
Requires=sound.target

[Service]
Type=simple
User=root
Group=audio
ExecStart=/usr/bin/radae-encoder --config /etc/radae/encoder.conf
Restart=on-failure
RestartSec=5s

# Performance tuning
CPUSchedulingPolicy=fifo
CPUSchedulingPriority=90
Nice=-10

# Resource limits
MemoryMax=512M
CPUQuota=200%

# Security
PrivateTmp=true
ProtectSystem=strict
ReadWritePaths=/var/lib/radae

# Environment
Environment="RADAE_BACKEND=npu"
Environment="RADAE_MODEL_DIR=/usr/share/radae/models"
EnvironmentFile=-/etc/radae/radae.env

[Install]
WantedBy=multi-user.target
```

### Service Recipe

```bitbake
SUMMARY = "RADAE Systemd Services"
LICENSE = "MIT"

SRC_URI = " \
    file://radae-encoder.service \
    file://radae-decoder.service \
    file://radae-monitor.service \
    file://radae.env \
"

S = "${WORKDIR}"

inherit systemd

SYSTEMD_SERVICE:${PN} = " \
    radae-encoder.service \
    radae-decoder.service \
    radae-monitor.service \
"

SYSTEMD_AUTO_ENABLE = "enable"

do_install() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/radae-encoder.service ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/radae-decoder.service ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/radae-monitor.service ${D}${systemd_system_unitdir}

    install -d ${D}${sysconfdir}/radae
    install -m 0644 ${WORKDIR}/radae.env ${D}${sysconfdir}/radae/
}

FILES:${PN} += "${systemd_system_unitdir}/*"

RDEPENDS:${PN} = "radae-core systemd"
```

## Package Groups

### RADAE Package Group

```bitbake
SUMMARY = "RADAE Package Group"
LICENSE = "MIT"

inherit packagegroup

RDEPENDS:${PN} = " \
    radae-core \
    radae-encoder \
    radae-decoder \
    radae-models \
    radae-tools \
    radae-service \
"

RRECOMMENDS:${PN} = " \
    radae-examples \
    radae-documentation \
"
```

## Init Scripts

For SysVinit (if not using systemd):

```bash
#!/bin/sh
### BEGIN INIT INFO
# Provides:          radae
# Required-Start:    $network $sound
# Required-Stop:     $network $sound
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: RADAE Service
### END INIT INFO

DAEMON=/usr/bin/radae-encoder
NAME=radae-encoder
PIDFILE=/var/run/$NAME.pid

case "$1" in
    start)
        echo "Starting $NAME"
        start-stop-daemon --start --quiet --pidfile $PIDFILE --exec $DAEMON
        ;;
    stop)
        echo "Stopping $NAME"
        start-stop-daemon --stop --quiet --pidfile $PIDFILE
        ;;
    restart)
        $0 stop
        $0 start
        ;;
    *)
        echo "Usage: $0 {start|stop|restart}"
        exit 1
        ;;
esac

exit 0
```

## Testing

On target device:
```bash
# Check systemd services
systemctl status radae-encoder
systemctl status radae-decoder

# View logs
journalctl -u radae-encoder -f

# Test NPU access
systemd-analyze security radae-encoder
```
