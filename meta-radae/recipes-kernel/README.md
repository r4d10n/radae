# recipes-kernel

This directory contains BitBake recipes for Linux kernel modifications.

## Purpose

Houses recipes for kernel-related components:

- **linux-imx**: Kernel patches and configurations for RADAE
- **kernel-modules**: Custom kernel modules
- **device-tree**: Device tree overlays and modifications
- **firmware**: Kernel firmware files

## Kernel Configuration

### Adding Kernel Config Fragments

Create configuration fragments for RADAE-specific features:

```
recipes-kernel/linux/
├── linux-imx_%.bbappend
└── files/
    ├── radae.cfg                    # Kernel config fragment
    └── imx8mp-radae.dts            # Device tree overlay
```

### Example: linux-imx_%.bbappend

```bitbake
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += "file://radae.cfg \
            file://imx8mp-radae.dts \
           "

# Add RADAE-specific kernel configuration
KERNEL_CONFIG_FRAGMENTS += "radae.cfg"

# Real-time kernel options (optional)
# SRC_URI += "file://radae-rt.cfg"

do_configure:append() {
    # Merge configuration fragments
    ${S}/scripts/kconfig/merge_config.sh -m -O ${B} ${B}/.config ${WORKDIR}/radae.cfg
}

# Add custom device tree
KERNEL_DEVICETREE:append = " freescale/imx8mp-radae.dtb"
```

### Kernel Config Fragment (radae.cfg)

```
# RADAE Kernel Configuration

# Audio support
CONFIG_SND=y
CONFIG_SND_SOC=y
CONFIG_SND_IMX_SOC=y
CONFIG_SND_SOC_FSL_SAI=y
CONFIG_SND_SOC_FSL_ASRC=y

# NPU support
CONFIG_GALCORE=m
CONFIG_VIVANTE_ENABLE_VG=y
CONFIG_VIVANTE_ENABLE_DRM=y

# Real-time features (optional)
# CONFIG_PREEMPT_RT=y
# CONFIG_HIGH_RES_TIMERS=y
# CONFIG_NO_HZ_FULL=y

# DMA and CMA
CONFIG_CMA=y
CONFIG_CMA_SIZE_MBYTES=320
CONFIG_DMA_CMA=y

# Performance monitoring
CONFIG_PERF_EVENTS=y
CONFIG_HW_PERF_EVENTS=y

# USB audio
CONFIG_SND_USB_AUDIO=m

# Networking for remote control
CONFIG_NETDEVICES=y
CONFIG_ETHERNET=y
```

## Device Tree Customization

### Device Tree Overlay Structure

```dts
/dts-v1/;
/plugin/;

#include "imx8mp.dtsi"

/ {
    compatible = "fsl,imx8mp-evk", "fsl,imx8mp";
    model = "NXP i.MX 8M Plus RADAE Board";
};

/* Audio configuration for RADAE */
&sai3 {
    status = "okay";
    pinctrl-names = "default";
    assigned-clocks = <&clk IMX8MP_CLK_SAI3>;
    assigned-clock-rates = <24576000>;
};

/* NPU reserved memory */
&reserved_memory {
    npu_reserved: npu@0x80000000 {
        compatible = "shared-dma-pool";
        reusable;
        size = <0 0x40000000>; /* 1GB for NPU */
        alignment = <0 0x1000>;
    };
};

/* NPU device */
&gpu_3d {
    status = "okay";
};
```

## Kernel Modules

### Custom Module Recipe

```bitbake
SUMMARY = "RADAE Custom Kernel Module"
LICENSE = "GPL-2.0"

inherit module

SRC_URI = "file://Makefile \
           file://radae_driver.c \
          "

S = "${WORKDIR}"

# Module parameters
MODULE_NAME = "radae_driver"
```

## Firmware Files

If custom firmware is needed:

```
recipes-kernel/firmware/
└── radae-firmware_1.0.bb
```

```bitbake
SUMMARY = "RADAE Firmware Files"
LICENSE = "Proprietary"

SRC_URI = "file://radae_npu.bin"

S = "${WORKDIR}"

do_install() {
    install -d ${D}${nonarch_base_libdir}/firmware
    install -m 0644 radae_npu.bin ${D}${nonarch_base_libdir}/firmware/
}

FILES:${PN} = "${nonarch_base_libdir}/firmware/*"
```

## Testing Kernel Changes

On target device:
```bash
# Check kernel version
uname -a

# List loaded modules
lsmod | grep -E "(galcore|radae)"

# View device tree
dtc -I fs /sys/firmware/devicetree/base > /tmp/devicetree.dts

# Check audio devices
cat /proc/asound/cards

# NPU status
cat /sys/kernel/debug/gc/info
```

## Debugging

Enable kernel debugging:
```bitbake
# In local.conf
EXTRA_IMAGE_FEATURES += "tools-debug"

# Kernel debug symbols
IMAGE_INSTALL:append = " kernel-dev kernel-devsrc"
```
