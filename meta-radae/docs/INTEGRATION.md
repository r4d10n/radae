# RADAE BSP Integration Guide

**Version:** 1.0
**Platform:** NXP i.MX 8M Plus
**Date:** 2025-11-17

## Table of Contents

1. [Overview](#overview)
2. [Adding to Existing BSP](#adding-to-existing-bsp)
3. [Machine Configuration](#machine-configuration)
4. [NPU Setup and Verification](#npu-setup-and-verification)
5. [Audio Configuration](#audio-configuration)
6. [RF Interface Setup](#rf-interface-setup)
7. [Custom Distributions](#custom-distributions)
8. [Device Tree Customization](#device-tree-customization)
9. [Kernel Configuration](#kernel-configuration)

---

## Overview

This guide describes how to integrate the meta-radae layer into existing i.MX 8M Plus Board Support Packages (BSPs) and customize the configuration for specific hardware setups.

### Integration Scenarios

- **Scenario A:** Adding to NXP's official i.MX Yocto BSP
- **Scenario B:** Adding to third-party board vendor BSP  
- **Scenario C:** Creating custom machine configuration
- **Scenario D:** Integrating with existing product BSP

---

## Adding to Existing BSP

### Scenario A: NXP Official i.MX BSP

If you're using NXP's official i.MX Yocto BSP release:

```bash
# Typical NXP BSP structure after repo init
imx-yocto-bsp/
├── sources/
│   ├── poky/
│   ├── meta-openembedded/
│   ├── meta-freescale/
│   └── meta-imx/
└── build/

# Add meta-radae layer
cd imx-yocto-bsp/sources
git clone <radae-repo> meta-radae

# Or copy from existing location
cp -r /path/to/radae/meta-radae .
```

Update build configuration:

```bash
cd imx-yocto-bsp
source setup-environment build-radae

# Add layer
bitbake-layers add-layer ../sources/meta-radae

# Configure local.conf
vi conf/local.conf
```

Add to `local.conf`:

```bash
# Use NXP's distribution
DISTRO = "fsl-imx-wayland"
MACHINE = "imx8mpevk"

# Accept EULA
ACCEPT_FSL_EULA = "1"

# Add RADAE packages
IMAGE_INSTALL:append = " radae-npu radae-npu-test"

# Ensure NPU support
MACHINEOVERRIDES =. "imxnpu:"
MACHINE_FEATURES:append = " npu"
```

### Scenario B: Third-Party Board Vendor BSP

Example: Variscite, Boundary Devices, Toradex, etc.

```bash
# Vendor BSPs typically provide their own setup script
cd <vendor-bsp>
./variscite-setup-release.sh  # Or vendor-specific script

# After setup
cd build
bitbake-layers add-layer ../../sources/meta-radae

# Check vendor's machine name
grep ^MACHINE conf/local.conf
# e.g., MACHINE = "var-som-mx8m-plus"

# Verify NPU support in machine config
bitbake -e | grep ^MACHINE_FEATURES=
# Should include 'npu' if supported
```

If NPU is not in `MACHINE_FEATURES`, add it:

```bash
# In local.conf
MACHINE_FEATURES:append = " npu"
```

### Scenario C: Custom Machine Configuration

Create a new machine configuration for your custom board:

```bash
cd meta-radae

# Create machine configuration directory if it doesn't exist
mkdir -p conf/machine

# Create machine config file
vi conf/machine/myboard-imx8mp.conf
```

Example `myboard-imx8mp.conf`:

```bitbake
#@TYPE: Machine
#@NAME: My Custom i.MX 8M Plus Board
#@SOC: i.MX8MP
#@DESCRIPTION: Custom board with i.MX 8M Plus
#@MAINTAINER: Your Name <your.email@example.com>

# Include i.MX 8M Plus common settings
require conf/machine/include/imx8mp-common.inc

# Kernel device tree
KERNEL_DEVICETREE = "freescale/myboard-imx8mp.dtb"

# U-Boot configuration
UBOOT_CONFIG ??= "sd"
UBOOT_CONFIG[sd] = "myboard_imx8mp_defconfig,sdcard"
UBOOT_CONFIG[fspi] = "myboard_imx8mp_defconfig"

# Boot device
IMXBOOT_TARGETS = "flash_evk"

# Serial console
SERIAL_CONSOLES = "115200;ttymxc1"

# Machine features
MACHINE_FEATURES += " \
    npu \
    vpu \
    gpu \
    wifi \
    bluetooth \
    alsa \
"

# Firmware
MACHINE_FIRMWARE:append = " \
    linux-firmware-imx-sdma-imx8mp \
    firmware-imx-vpu-imx8 \
    firmware-imx-epdc \
"

# Preferred providers
PREFERRED_PROVIDER_virtual/kernel = "linux-imx"
PREFERRED_PROVIDER_virtual/bootloader = "u-boot-imx"
PREFERRED_PROVIDER_u-boot = "u-boot-imx"

# Image format
IMAGE_FSTYPES = "wic.zst tar.gz"
WKS_FILE = "imx8mp-sdcard.wks"
```

Use the custom machine:

```bash
# In local.conf
MACHINE = "myboard-imx8mp"
```

---

## Machine Configuration

### Verifying Machine Features

```bash
# Check current machine features
bitbake -e | grep ^MACHINE_FEATURES=

# Expected output should include:
# MACHINE_FEATURES="alsa usbgadget usbhost vfat ext2 npu vpu gpu ..."
```

### Enabling/Disabling Features

```bash
# In local.conf or machine config

# Add features
MACHINE_FEATURES:append = " npu gpu vpu"

# Remove features
MACHINE_FEATURES:remove = " x11"

# Override completely (not recommended)
MACHINE_FEATURES = "alsa usbhost npu"
```

### NPU-Specific Machine Configuration

Ensure these settings for NPU support:

```bash
# In machine config or local.conf

# NPU override
MACHINEOVERRIDES =. "imxnpu:"

# NPU feature
MACHINE_FEATURES:append = " npu"

# VeriSilicon NPU firmware
MACHINE_FIRMWARE:append = " firmware-imx-vpu-imx8"

# Preferred GPU driver (includes NPU)
PREFERRED_PROVIDER_virtual/libg2d = "imx-gpu-g2d"
PREFERRED_PROVIDER_virtual/libgles1 = "imx-gpu-viv"
PREFERRED_PROVIDER_virtual/libgles2 = "imx-gpu-viv"
PREFERRED_PROVIDER_virtual/egl = "imx-gpu-viv"
PREFERRED_PROVIDER_virtual/libgl = "imx-gpu-viv"
```

### Audio Configuration in Machine

```bash
# Add ALSA support
MACHINE_FEATURES:append = " alsa"

# Specify sound card driver
MACHINE_EXTRA_RDEPENDS:append = " \
    alsa-utils \
    alsa-plugins \
"

# For specific codec (e.g., WM8960)
MACHINE_EXTRA_RRECOMMENDS:append = " kernel-module-snd-soc-wm8960"
```

---

## NPU Setup and Verification

### NPU Driver Installation

The NPU driver is provided by NXP's `imx-vsi-npu` package.

**Automatic (Recommended):**

```bash
# In local.conf
IMAGE_INSTALL:append = " imx-vsi-npu"
```

**Manual Configuration:**

```bash
# Add NPU support recipes
meta-radae/recipes-ml/imx-vsi-npu/
```

### NPU Firmware

```bash
# Ensure NPU firmware is installed
IMAGE_INSTALL:append = " \
    firmware-imx-vpu-imx8 \
    imx-vpu-hantro \
"
```

### Kernel Configuration for NPU

The NPU requires specific kernel drivers. Verify in kernel config:

```bash
# Check kernel configuration
bitbake virtual/kernel -c menuconfig

# Navigate to:
# Device Drivers -> Multimedia support -> VeriSilicon ISP driver
# Device Drivers -> Misc devices -> VeriSilicon NPU driver

# Required configs:
CONFIG_VIDEO_HANTRO_IMX8M=y
CONFIG_GALCORE=m
CONFIG_GPU_VIV_V7=y
```

Or add via kernel recipe:

```bash
# meta-radae/recipes-kernel/linux/linux-imx_%.bbappend
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " file://npu.cfg"
```

Create `npu.cfg`:

```
CONFIG_GALCORE=m
CONFIG_GPU_VIV_V7=y
CONFIG_VIDEO_HANTRO_IMX8M=y
```

### Device Tree Configuration for NPU

The NPU device tree node should be enabled. Verify/add:

```dts
&npu {
    status = "okay";
};

&galcore {
    status = "okay";
};
```

### Post-Boot NPU Verification

On target device after boot:

```bash
# Check NPU device node
ls -l /dev/galcore
# Expected: crw-rw---- 1 root video 249, 0 ...

# Check NPU kernel module
lsmod | grep galcore
# Expected: galcore 524288 0

# Check NPU driver version
cat /sys/class/misc/galcore/device/driver/version
# Expected: GC kernel version: 6.4.x

# Check NPU firmware
dmesg | grep -i galcore
# Expected: galcore: Galcore version 6.4.x.xxxxxx

# Test NPU with TensorFlow Lite
python3 << EOF
import tflite_runtime.interpreter as tflite
import numpy as np

# Try loading NPU delegate
try:
    delegate = tflite.load_delegate('/usr/lib/libvx_delegate.so')
    print("NPU delegate loaded successfully!")
except Exception as e:
    print(f"Failed to load NPU delegate: {e}")
EOF
```

### Troubleshooting NPU Issues

**NPU device not found:**

```bash
# Check kernel module load
dmesg | grep galcore

# Manual load
modprobe galcore

# Check for errors
journalctl | grep galcore
```

**NPU permissions:**

```bash
# Add user to video group
usermod -a -G video root

# Or modify udev rules
# /etc/udev/rules.d/99-galcore.rules
KERNEL=="galcore", MODE="0666"
```

**NPU delegate not found:**

```bash
# Check installation
find /usr/lib -name "libvx_delegate.so"

# If missing, ensure package is installed
opkg install tensorflow-lite-vx-delegate
# or
dnf install tensorflow-lite-vx-delegate
```

---

## Audio Configuration

### ALSA Configuration

#### Default ALSA Config

```bash
# Install ALSA utilities
IMAGE_INSTALL:append = " \
    alsa-utils \
    alsa-lib \
    alsa-plugins \
"
```

#### List Audio Devices

On target:

```bash
# List playback devices
aplay -l
# Expected output:
# card 0: wm8960audio [wm8960-audio], device 0: HiFi wm8960-hifi-0 []

# List capture devices
arecord -l

# List all devices with details
cat /proc/asound/cards
```

#### Test Audio Playback

```bash
# Speaker test
speaker-test -c 2 -t wav

# Play WAV file
aplay /usr/share/sounds/alsa/Front_Center.wav

# Record audio
arecord -f S16_LE -r 16000 -c 1 -d 10 test.wav

# Play recorded audio
aplay test.wav
```

#### RADAE Audio Configuration

RADAE requires specific audio settings for HF radio:

```bash
# Configure ALSA for RADAE
# /etc/asound.conf
pcm.radae_tx {
    type hw
    card 0
    device 0
    channels 1
    rate 16000
    format S16_LE
}

pcm.radae_rx {
    type hw
    card 0
    device 0
    channels 1
    rate 16000
    format S16_LE
}
```

Test RADAE audio:

```bash
# Capture from radio input
arecord -D radae_rx -f S16_LE -r 16000 -c 1 -d 10 radio_input.wav

# Play to radio output
aplay -D radae_tx -f S16_LE -r 16000 -c 1 radio_output.wav
```

### PulseAudio Configuration (Optional)

For more advanced audio routing:

```bash
# In local.conf
DISTRO_FEATURES:append = " pulseaudio"
IMAGE_INSTALL:append = " pulseaudio-server pulseaudio-misc"
```

PulseAudio configuration for RADAE:

```bash
# /etc/pulse/default.pa

# Load RADAE source (radio input)
load-module module-alsa-source device=hw:0,0 source_name=radae_source channels=1 rate=16000 format=s16le

# Load RADAE sink (radio output)
load-module module-alsa-sink device=hw:0,0 sink_name=radae_sink channels=1 rate=16000 format=s16le

# Set defaults
set-default-source radae_source
set-default-sink radae_sink
```

### Codec-Specific Configuration

For WM8960 codec (common on i.MX 8M Plus):

```bash
# Kernel module
modprobe snd_soc_wm8960

# ALSA mixer settings
amixer -c 0 sset 'Left Input Mixer Boost' on
amixer -c 0 sset 'Right Input Mixer Boost' on
amixer -c 0 sset 'Headphone' 100
amixer -c 0 sset 'Speaker' 100

# Save mixer settings
alsactl store
```

---

## RF Interface Setup

### GPIO Configuration for PTT

Push-To-Talk (PTT) control via GPIO:

#### Device Tree Configuration

```dts
/* In custom device tree */
/ {
    radae {
        compatible = "gpio-keys";
        
        ptt {
            label = "PTT";
            gpios = <&gpio5 3 GPIO_ACTIVE_HIGH>;
            linux,code = <KEY_F12>;  /* Or custom keycode */
        };
    };
    
    ptt-control {
        compatible = "gpio-leds";
        
        ptt-led {
            label = "ptt";
            gpios = <&gpio5 4 GPIO_ACTIVE_HIGH>;
            default-state = "off";
        };
    };
};
```

#### User-Space PTT Control

```bash
# Export GPIO
echo 131 > /sys/class/gpio/export  # GPIO5_3 = 5*32 + 3 = 131

# Set as output
echo out > /sys/class/gpio/gpio131/direction

# PTT on (transmit)
echo 1 > /sys/class/gpio/gpio131/value

# PTT off (receive)
echo 0 > /sys/class/gpio/gpio131/value
```

#### RADAE PTT Integration

```c
// Example PTT control in RADAE application
#include <fcntl.h>
#include <unistd.h>

#define PTT_GPIO "/sys/class/gpio/gpio131/value"

void ptt_enable() {
    int fd = open(PTT_GPIO, O_WRONLY);
    write(fd, "1", 1);
    close(fd);
}

void ptt_disable() {
    int fd = open(PTT_GPIO, O_WRONLY);
    write(fd, "0", 1);
    close(fd);
}
```

### SPI/I2C RF Transceiver Integration

For RF transceivers like Si446x, ADF7021, etc.:

#### Device Tree for SPI

```dts
&ecspi1 {
    fsl,spi-num-chipselects = <1>;
    cs-gpios = <&gpio5 9 GPIO_ACTIVE_LOW>;
    pinctrl-names = "default";
    pinctrl-0 = <&pinctrl_ecspi1>;
    status = "okay";
    
    rf_transceiver: si4463@0 {
        compatible = "silabs,si4463";
        reg = <0>;
        spi-max-frequency = <10000000>;
        interrupt-parent = <&gpio5>;
        interrupts = <10 IRQ_TYPE_EDGE_FALLING>;
        
        /* RF-specific properties */
        tx-frequency = <14236000>;  /* 14.236 MHz (20m band) */
        bandwidth = <1500>;          /* 1.5 kHz */
        tx-power = <10>;             /* 10 dBm */
    };
};
```

### USB SDR Integration

For USB Software-Defined Radios (RTL-SDR, HackRF, PlutoSDR):

```bash
# In local.conf
IMAGE_INSTALL:append = " \
    libusb1 \
    rtl-sdr \
    hackrf \
    libiio \
"

# Add user to plugdev group
EXTRA_USERS_PARAMS = "usermod -a -G plugdev root;"
```

Test USB SDR on target:

```bash
# RTL-SDR
rtl_test -t

# HackRF
hackrf_info

# PlutoSDR (via libiio)
iio_info
iio_attr -C ad9361-phy RX_LO frequency
```

---

## Custom Distributions

### Creating Custom RADAE Distribution

```bash
# meta-radae/conf/distro/radae-distro.conf
require conf/distro/poky.conf

DISTRO = "radae-distro"
DISTRO_NAME = "RADAE Embedded Linux"
DISTRO_VERSION = "1.0"
DISTRO_CODENAME = "alpha"

# Use systemd
DISTRO_FEATURES:append = " systemd"
DISTRO_FEATURES_BACKFILL_CONSIDERED:append = " sysvinit"
VIRTUAL-RUNTIME_init_manager = "systemd"
VIRTUAL-RUNTIME_initscripts = ""

# Enable Wayland (no X11)
DISTRO_FEATURES:append = " wayland"
DISTRO_FEATURES:remove = " x11"

# Networking
DISTRO_FEATURES:append = " wifi bluetooth"

# Security
DISTRO_FEATURES:append = " pam"

# Audio
DISTRO_FEATURES:append = " alsa pulseaudio"

# Optimization
DISTRO_FEATURES:append = " performance"

# NPU support
DISTRO_FEATURES:append = " npu-support"

# Package management
DISTRO_FEATURES:append = " package-management"
PACKAGE_CLASSES = "package_rpm"

# Default preferred versions
PREFERRED_VERSION_tensorflow-lite = "2.12%"
PREFERRED_VERSION_python3 = "3.10%"

# Toolchain
TCLIBC = "glibc"
SDKIMAGE_FEATURES:append = " dev-pkgs dbg-pkgs"
```

Use custom distribution:

```bash
# In local.conf
DISTRO = "radae-distro"
```

### Distribution Features

Common features to enable/disable:

```bash
# Minimal distribution
DISTRO_FEATURES = "systemd alsa usbhost ${DISTRO_FEATURES_LIBC}"

# Full-featured
DISTRO_FEATURES:append = " \
    systemd \
    wayland \
    wifi \
    bluetooth \
    alsa \
    pulseaudio \
    pam \
    ipv6 \
    opengl \
"

# Remove unwanted features
DISTRO_FEATURES:remove = " x11 vulkan nfs zeroconf 3g"
```

---

## Device Tree Customization

### Adding Custom Device Tree

```bash
# Create device tree directory
mkdir -p meta-radae/recipes-kernel/linux/linux-imx/

# Create device tree file
vi meta-radae/recipes-kernel/linux/linux-imx/myboard-radae.dts
```

Example device tree:

```dts
/* myboard-radae.dts */
#include "imx8mp-evk.dts"

/ {
    model = "My Board RADAE";
    compatible = "mycompany,myboard-radae", "fsl,imx8mp";
    
    /* RADAE-specific nodes */
    radae {
        compatible = "radae,audio-codec";
        status = "okay";
        
        audio-input = <&audio_codec 0>;
        audio-output = <&audio_codec 1>;
        sample-rate = <16000>;
        channels = <1>;
    };
};

/* Enable NPU */
&npu {
    status = "okay";
};

/* Configure audio codec */
&i2c3 {
    wm8960: codec@1a {
        compatible = "wlf,wm8960";
        reg = <0x1a>;
        clocks = <&clk IMX8MP_CLK_SAI3>;
        clock-names = "mclk";
        wlf,shared-lrclk;
    };
};

/* PTT GPIO */
&gpio5 {
    ptt_pin {
        gpio-hog;
        gpios = <3 GPIO_ACTIVE_HIGH>;
        output-low;
        line-name = "ptt-control";
    };
};
```

Append to kernel recipe:

```bash
# meta-radae/recipes-kernel/linux/linux-imx_%.bbappend
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://myboard-radae.dts \
"

# Add to devicetree build
do_compile:append() {
    cp ${WORKDIR}/myboard-radae.dts ${S}/arch/arm64/boot/dts/freescale/
}
```

---

## Kernel Configuration

### Kernel Configuration Fragments

Create kernel config fragment:

```bash
# meta-radae/recipes-kernel/linux/linux-imx/radae.cfg

# RADAE-specific kernel options
CONFIG_SOUND=y
CONFIG_SND=y
CONFIG_SND_SOC=y
CONFIG_SND_SOC_FSL_SAI=y
CONFIG_SND_SOC_WM8960=m

# NPU support
CONFIG_GALCORE=m
CONFIG_GPU_VIV_V7=y

# USB Audio (for testing)
CONFIG_SND_USB_AUDIO=m

# GPIO
CONFIG_GPIO_SYSFS=y

# SPI (for RF transceivers)
CONFIG_SPI=y
CONFIG_SPI_IMX=y

# I2C (for audio codecs)
CONFIG_I2C=y
CONFIG_I2C_IMX=y

# Real-time preemption (optional)
CONFIG_PREEMPT=y
# CONFIG_PREEMPT_RT=y  # For RT kernel
```

Add to kernel recipe:

```bash
# meta-radae/recipes-kernel/linux/linux-imx_%.bbappend
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " file://radae.cfg"
```

### Kernel Patches

Apply kernel patches if needed:

```bash
# meta-radae/recipes-kernel/linux/linux-imx/0001-add-radae-support.patch

# bbappend
SRC_URI:append = " file://0001-add-radae-support.patch"
```

---

## Summary

This guide covered:
- Adding meta-radae to existing BSPs
- Machine configuration for custom boards
- NPU driver setup and verification
- Audio interface configuration for HF radio
- RF transceiver integration (GPIO PTT, SPI/USB)
- Custom distribution creation
- Device tree customization
- Kernel configuration

For detailed build instructions, see [BUILD_GUIDE.md](BUILD_GUIDE.md).
For testing procedures, see [TESTING.md](TESTING.md).
