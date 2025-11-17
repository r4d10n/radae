# RADAE Yocto Build Guide

**Version:** 1.0
**Platform:** NXP i.MX 8M Plus
**Date:** 2025-11-17

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Environment Setup](#environment-setup)
3. [Layer Configuration](#layer-configuration)
4. [Build Configuration](#build-configuration)
5. [Building Images](#building-images)
6. [Flashing and Deployment](#flashing-and-deployment)
7. [Advanced Build Options](#advanced-build-options)
8. [Troubleshooting](#troubleshooting)

---

## Prerequisites

### Host System Requirements

**Recommended:**
- **OS:** Ubuntu 22.04 LTS (64-bit)
- **CPU:** 4+ cores (8+ recommended)
- **RAM:** 16 GB minimum, 32 GB recommended
- **Disk Space:** 100 GB free minimum, 200 GB recommended
- **Network:** High-speed internet for downloads

**Also Supported:**
- Ubuntu 20.04 LTS
- Fedora 36+
- Debian 11+
- OpenSUSE Leap 15.3+

### Install Required Packages

#### Ubuntu/Debian

```bash
sudo apt update
sudo apt install -y \
    gawk wget git diffstat unzip texinfo \
    gcc build-essential chrpath socat cpio \
    python3 python3-pip python3-pexpect \
    xz-utils debianutils iputils-ping \
    python3-git python3-jinja2 libegl1-mesa \
    libsdl1.2-dev pylint3 xterm python3-subunit \
    mesa-common-dev zstd liblz4-tool file locales \
    libacl1

# Install additional tools
sudo apt install -y \
    bmap-tools parted dosfstools mtools \
    repo git-lfs curl lz4
```

#### Fedora

```bash
sudo dnf install -y \
    gawk make wget tar bzip2 gzip python3 unzip \
    perl patch diffutils diffstat git cpp gcc gcc-c++ \
    glibc-devel texinfo chrpath ccache perl-Data-Dumper \
    perl-Text-ParseWords perl-Thread-Queue python3-pip \
    xz which SDL-devel xterm zstd lz4 file
```

### Configure Git

```bash
git config --global user.name "Your Name"
git config --global user.email "your.email@example.com"
```

### Verify Locale

```bash
# Check current locale
locale

# If not UTF-8, configure:
sudo locale-gen en_US.UTF-8
sudo update-locale LC_ALL=en_US.UTF-8 LANG=en_US.UTF-8
```

---

## Environment Setup

### 1. Create Workspace

```bash
# Create dedicated workspace
mkdir -p ~/yocto-radae
cd ~/yocto-radae

# Set workspace variable for convenience
export YOCTO_ROOT=~/yocto-radae
```

### 2. Clone Yocto/Poky (LTS Branch)

```bash
cd $YOCTO_ROOT

# Clone Yocto Project reference distribution (kirkstone LTS)
git clone -b kirkstone git://git.yoctoproject.org/poky.git

# Expected output:
# Cloning into 'poky'...
# remote: Counting objects: ...
```

**Alternative: Use repo Tool**

```bash
# Install repo tool
mkdir -p ~/bin
curl https://storage.googleapis.com/git-repo-downloads/repo > ~/bin/repo
chmod a+x ~/bin/repo
export PATH=~/bin:$PATH

# Initialize repo manifest (if using NXP BSP manifest)
mkdir imx-yocto-bsp
cd imx-yocto-bsp
repo init -u https://github.com/nxp-imx/imx-manifest -b imx-linux-kirkstone -m imx-5.15.52-2.1.0.xml
repo sync
```

### 3. Clone Required Layers

```bash
cd $YOCTO_ROOT/poky

# Clone meta-freescale (NXP i.MX BSP)
git clone -b kirkstone git://git.yoctoproject.org/meta-freescale

# Clone meta-freescale-3rdparty (additional board support)
git clone -b kirkstone https://github.com/Freescale/meta-freescale-3rdparty

# Clone meta-openembedded (essential dependencies)
git clone -b kirkstone https://git.openembedded.org/meta-openembedded

# Clone meta-ml (TensorFlow Lite, NPU support)
git clone -b kirkstone https://git.yoctoproject.org/meta-ml

# Or use NXP's meta-imx (includes meta-ml)
git clone -b kirkstone https://github.com/nxp-imx/meta-imx
```

### 4. Add meta-radae Layer

```bash
cd $YOCTO_ROOT/poky

# Option 1: Clone from repository
git clone <radae-repository-url> meta-radae

# Option 2: Copy from existing location
cp -r /path/to/radae/meta-radae .

# Option 3: Symlink (for development)
ln -s /path/to/radae/meta-radae .
```

### 5. Verify Layer Structure

```bash
ls -l $YOCTO_ROOT/poky/
# Should show:
# drwxr-xr-x meta
# drwxr-xr-x meta-poky
# drwxr-xr-x meta-yocto-bsp
# drwxr-xr-x meta-freescale
# drwxr-xr-x meta-freescale-3rdparty
# drwxr-xr-x meta-openembedded
# drwxr-xr-x meta-ml
# drwxr-xr-x meta-radae
```

---

## Layer Configuration

### 1. Initialize Build Environment

```bash
cd $YOCTO_ROOT/poky

# Source OpenEmbedded build environment
source oe-init-build-env build-imx8mp

# This creates build-imx8mp/ directory and changes to it
# Expected output:
# You had no conf/local.conf file. This configuration file has therefore been
# created for you with some default values...
```

**Important:** You must source `oe-init-build-env` every time you start a new shell session.

### 2. Add Layers to bblayers.conf

Method 1: Using bitbake-layers command (recommended)

```bash
cd $YOCTO_ROOT/poky/build-imx8mp

# Add meta-oe (OpenEmbedded core)
bitbake-layers add-layer ../meta-openembedded/meta-oe

# Add meta-python (Python recipes)
bitbake-layers add-layer ../meta-openembedded/meta-python

# Add meta-networking (network utilities)
bitbake-layers add-layer ../meta-openembedded/meta-networking

# Add meta-freescale (NXP i.MX BSP)
bitbake-layers add-layer ../meta-freescale

# Add meta-freescale-3rdparty (board-specific)
bitbake-layers add-layer ../meta-freescale-3rdparty

# Add meta-ml (TensorFlow Lite)
bitbake-layers add-layer ../meta-ml

# Add meta-radae (RADAE layer)
bitbake-layers add-layer ../meta-radae

# Verify layers
bitbake-layers show-layers
```

Method 2: Edit conf/bblayers.conf manually

```bash
vi conf/bblayers.conf
```

Add the following layers:

```python
# POKY_BBLAYERS_CONF_VERSION is increased each time build/conf/bblayers.conf
# changes incompatibly
POKY_BBLAYERS_CONF_VERSION = "2"

BBPATH = "${TOPDIR}"
BBFILES ?= ""

BSPDIR := "${@os.path.abspath(os.path.dirname(d.getVar('FILE', True)) + '/../..')}"

BBLAYERS ?= " \
  ${BSPDIR}/sources/poky/meta \
  ${BSPDIR}/sources/poky/meta-poky \
  ${BSPDIR}/sources/poky/meta-yocto-bsp \
  ${BSPDIR}/sources/meta-openembedded/meta-oe \
  ${BSPDIR}/sources/meta-openembedded/meta-python \
  ${BSPDIR}/sources/meta-openembedded/meta-networking \
  ${BSPDIR}/sources/meta-freescale \
  ${BSPDIR}/sources/meta-freescale-3rdparty \
  ${BSPDIR}/sources/meta-ml \
  ${BSPDIR}/sources/meta-radae \
"
```

### 3. Verify Layer Dependencies

```bash
# Check for missing dependencies
bitbake-layers show-layers

# Expected output should show all layers without errors
```

---

## Build Configuration

### 1. Configure local.conf

The main build configuration file is `conf/local.conf`. You can:
- Start from scratch and add settings
- Use the sample: `cp ../meta-radae/conf/local.conf.sample conf/local.conf`

Edit `conf/local.conf`:

```bash
vi conf/local.conf
```

### 2. Essential Settings

#### Machine Configuration

```bash
# Set target machine to i.MX 8M Plus EVK
MACHINE = "imx8mpevk"

# Or for other i.MX 8M Plus boards:
# MACHINE = "imx8mp-lpddr4-evk"     # LPDDR4 variant
# MACHINE = "var-som-mx8m-plus"     # Variscite SOM
# MACHINE = "nitrogen8mp"           # Boundary Devices
```

#### Distribution

```bash
# Use NXP's Wayland-based distribution
DISTRO = "fsl-imx-wayland"

# Or use minimal:
# DISTRO = "poky"
```

#### Accept NXP EULA

```bash
# Required for NXP proprietary firmware
ACCEPT_FSL_EULA = "1"
```

#### Enable NPU Support

```bash
# Add NPU to machine overrides
MACHINEOVERRIDES =. "use-nxp-bsp:imx-nxp-bsp:imxvpuhantro:imxvpug2:imxgpu2d:imxgpu3d:imxipu:imxvpucnm:imxnpu:"

# Enable NPU in machine features
MACHINE_FEATURES:append = " npu"
```

#### Parallel Build Settings

```bash
# Number of bitbake threads (number of parallel tasks)
# Set to number of CPU cores
BB_NUMBER_THREADS = "8"

# Number of parallel make jobs per task
# Set to number of CPU cores or cores + 2
PARALLEL_MAKE = "-j 8"
```

#### Download Directory

```bash
# Shared download directory (can be reused across builds)
DL_DIR = "${TOPDIR}/../downloads"
```

#### Shared State Cache

```bash
# Shared state cache (speeds up rebuilds)
SSTATE_DIR = "${TOPDIR}/../sstate-cache"
```

#### Disk Space Monitoring

```bash
BB_DISKMON_DIRS = "\
    STOPTASKS,${TMPDIR},1G,100K \
    STOPTASKS,${DL_DIR},1G,100K \
    STOPTASKS,${SSTATE_DIR},1G,100K \
    ABORT,${TMPDIR},100M,1K \
    ABORT,${DL_DIR},100M,1K \
    ABORT,${SSTATE_DIR},100M,1K \
"
```

### 3. RADAE-Specific Configuration

```bash
# Add RADAE packages to image
IMAGE_INSTALL:append = " \
    radae-npu \
    radae-npu-test \
    tensorflow-lite \
    imx-vsi-npu \
"

# Enable systemd (recommended for RADAE)
DISTRO_FEATURES:append = " systemd"
VIRTUAL-RUNTIME_init_manager = "systemd"
VIRTUAL-RUNTIME_initscripts = "systemd-compat-units"

# Enable Wayland (optional, for UI)
DISTRO_FEATURES:append = " wayland"

# Package management (for development)
EXTRA_IMAGE_FEATURES:append = " package-management"

# Debug packages (optional)
# EXTRA_IMAGE_FEATURES:append = " dbg-pkgs dev-pkgs"

# SSH server for remote access
EXTRA_IMAGE_FEATURES:append = " ssh-server-openssh"
```

### 4. Development vs Production Builds

#### Development Build

```bash
# Enable debug and development tools
EXTRA_IMAGE_FEATURES:append = " \
    debug-tweaks \
    tools-debug \
    tools-sdk \
    dev-pkgs \
    dbg-pkgs \
"

# Allow empty root password
EXTRA_IMAGE_FEATURES:append = " empty-root-password"

# Include compiler and build tools on target
TOOLCHAIN_TARGET_TASK:append = " \
    gcc \
    g++ \
    make \
    cmake \
"
```

#### Production Build

```bash
# Minimal features
EXTRA_IMAGE_FEATURES = "ssh-server-openssh"

# Remove debug symbols
INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "0"

# Remove documentation
INHERIT += "rm_work"
```

### 5. Optimization Settings

```bash
# Optimize for size (smaller images)
# DISTRO_FEATURES:append = " size-optimization"

# Optimize for performance (faster runtime)
DISTRO_FEATURES:append = " performance"

# ARM-specific optimizations
DEFAULTTUNE = "armv8a-crc-crypto"

# Enable Link-Time Optimization (LTO)
# DISTRO_FEATURES:append = " lto"
```

---

## Building Images

### 1. Build Minimal RADAE Image

```bash
cd $YOCTO_ROOT/poky/build-imx8mp

# Build core-image-minimal with RADAE
bitbake core-image-minimal
```

**Build Process:**
1. Parses recipes and dependencies
2. Downloads source code to `downloads/`
3. Builds toolchain
4. Builds all packages
5. Creates root filesystem
6. Generates bootloader and kernel
7. Creates flashable image

**Expected Build Time:**
- First build: **3-6 hours** (with 8 cores, fast internet)
- Incremental rebuilds: **5-30 minutes**

### 2. Monitor Build Progress

```bash
# In another terminal, watch build progress
tail -f tmp/log/cooker/qemux86-64/console-latest.log

# Or use bitbake's progress UI
bitbake -u taskexp core-image-minimal
```

### 3. Build Other Images

#### Full Command-Line Image

```bash
# Includes more utilities and tools
bitbake core-image-full-cmdline
```

#### Custom RADAE Image

Create `meta-radae/recipes-core/images/radae-image.bb`:

```bitbake
require recipes-core/images/core-image-minimal.bb

DESCRIPTION = "RADAE embedded image with NPU support"

IMAGE_INSTALL:append = " \
    radae-npu \
    radae-npu-test \
    tensorflow-lite \
    imx-vsi-npu \
    alsa-utils \
    pulseaudio \
    vim \
    htop \
    iperf3 \
"

IMAGE_FEATURES:append = " ssh-server-openssh package-management"
```

Then build:

```bash
bitbake radae-image
```

### 4. Build Individual Packages

```bash
# Build RADAE NPU package only
bitbake radae-npu

# Build TensorFlow Lite
bitbake tensorflow-lite

# Build NPU driver
bitbake imx-vsi-npu

# Build kernel
bitbake virtual/kernel

# Build bootloader
bitbake virtual/bootloader
```

### 5. Clean Builds

```bash
# Clean specific package (keeps downloads and sstate)
bitbake -c clean radae-npu

# Clean and remove from sstate
bitbake -c cleansstate radae-npu

# Remove everything including downloads
bitbake -c cleanall radae-npu

# Clean entire build (nuclear option)
rm -rf tmp/
```

---

## Flashing and Deployment

### 1. Locate Build Artifacts

After successful build, find images in:

```bash
cd tmp/deploy/images/imx8mpevk/
ls -lh

# Key files:
# imx-boot-imx8mpevk-sd.bin-flash_evk    # Bootloader (imx-boot)
# core-image-minimal-imx8mpevk.wic.zst   # Complete SD card image (compressed)
# core-image-minimal-imx8mpevk.wic       # Uncompressed image
# core-image-minimal-imx8mpevk.rootfs.tar.gz  # Root filesystem archive
```

### 2. Flash to SD Card (Complete Image)

#### Method 1: Using bmaptool (Recommended)

```bash
# Install bmaptool
sudo apt install bmap-tools

# Flash compressed image directly (fast!)
sudo bmaptool copy core-image-minimal-imx8mpevk.wic.zst /dev/sdX

# This uses block map for sparse writing, much faster than dd
```

#### Method 2: Using dd

```bash
# Decompress image first
zstd -d core-image-minimal-imx8mpevk.wic.zst

# Write to SD card (CAUTION: verify /dev/sdX is correct!)
sudo dd if=core-image-minimal-imx8mpevk.wic of=/dev/sdX bs=4M conv=fsync status=progress

# Sync to ensure all data is written
sudo sync
```

**Important:** Replace `/dev/sdX` with your actual SD card device. Use `lsblk` to identify:

```bash
lsblk
# Look for your SD card (e.g., /dev/sdb, /dev/mmcblk0)
```

### 3. Flash to eMMC (On-Board Storage)

Boot from SD card or serial download mode, then from Linux on target:

```bash
# On target device
# Flash bootloader to eMMC boot partition
echo 0 > /sys/block/mmcblk2boot0/force_ro
dd if=/media/imx-boot of=/dev/mmcblk2boot0 bs=1K seek=32 conv=fsync

# Flash rootfs to eMMC user partition
dd if=/media/core-image-minimal.wic of=/dev/mmcblk2 bs=4M conv=fsync status=progress

# Set boot partition
mmc bootpart enable 1 1 /dev/mmcblk2
```

### 4. Boot from SD Card

1. Insert SD card into i.MX 8M Plus board
2. Set boot switches to SD card mode (consult board manual)
3. Connect serial console (115200 8N1)
4. Power on board
5. Watch boot messages on serial console

**Expected Boot Sequence:**
```
U-Boot 2022.04 (...)
Starting kernel ...
[    0.000000] Booting Linux on physical CPU 0x0
...
[  OK  ] Started Serial Getty on ttymxc1.
[  OK  ] Reached target Login Prompts.

imx8mpevk login: root
```

### 5. Verify RADAE on Target

```bash
# Check NPU device
ls -l /dev/galcore

# Check RADAE installation
which radae-npu-test

# Run quick test
radae-npu-test --version

# Check NPU driver version
cat /sys/class/misc/galcore/device/driver/version
```

---

## Advanced Build Options

### 1. Using SDK

Build standalone SDK for cross-compilation:

```bash
# Build SDK installer
bitbake core-image-minimal -c populate_sdk

# SDK will be in:
# tmp/deploy/sdk/fsl-imx-wayland-glibc-x86_64-core-image-minimal-armv8a-imx8mpevk-toolchain-5.15-kirkstone.sh

# Install SDK on development PC
cd tmp/deploy/sdk/
./fsl-imx-wayland-glibc-x86_64-core-image-minimal-armv8a-imx8mpevk-toolchain-5.15-kirkstone.sh

# Default installation: /opt/fsl-imx-wayland/5.15-kirkstone/

# Use SDK
source /opt/fsl-imx-wayland/5.15-kirkstone/environment-setup-armv8a-poky-linux

# Verify cross-compiler
$CC --version
# Output: aarch64-poky-linux-gcc (GCC) 11.x
```

### 2. Using devtool

Rapid iterative development:

```bash
# Extract package source to workspace
devtool modify radae-npu

# Edit source in workspace
cd workspace/sources/radae-npu
# Make changes...

# Build and test
devtool build radae-npu

# Deploy to target over network
devtool deploy-target radae-npu root@192.168.1.100

# Test on target
# ssh root@192.168.1.100
# radae-npu-test ...

# Update recipe with changes
devtool update-recipe radae-npu

# Reset workspace
devtool reset radae-npu
```

### 3. Binary Package Feeds

Create package repository for OTA updates:

```bash
# Build package feed
bitbake package-index

# Serve packages over HTTP
cd tmp/deploy/rpm
python3 -m http.server 8000

# On target, add repository
# Edit /etc/yum.repos.d/oe-remote-repo.repo
[oe-remote]
name=OE Remote Repo
baseurl=http://192.168.1.10:8000/armv8a/
enabled=1
gpgcheck=0

# Install/update packages on target
dnf install radae-npu
dnf update
```

### 4. Cross-Compile Kernel Modules

```bash
# Build kernel SDK
bitbake virtual/kernel -c devshell

# In devshell, cross-compile modules
make ARCH=arm64 CROSS_COMPILE=aarch64-poky-linux- modules
```

### 5. Custom Image with Wic

Create custom partitioning scheme:

Edit `meta-radae/wic/radae-sdcard.wks`:

```wks
# short-description: Create SD card image with custom partitions
# long-description: Creates a SD card image with bootloader, boot, and rootfs partitions

part u-boot --source rawcopy --sourceparams="file=imx-boot" --ondisk mmcblk --no-table --align 1
part /boot --source bootimg-partition --ondisk mmcblk --fstype=vfat --label boot --active --align 4096 --size 64M
part / --source rootfs --ondisk mmcblk --fstype=ext4 --label root --align 4096 --size 2048M
part /data --ondisk mmcblk --fstype=ext4 --label data --align 4096 --size 1024M
```

Use in image recipe:

```bitbake
WKS_FILE = "radae-sdcard.wks"
```

---

## Troubleshooting

### Build Failures

#### 1. "No rule to make target"

```bash
# Often due to missing dependencies
# Solution: Clean and rebuild
bitbake -c cleanall <package>
bitbake <package>
```

#### 2. "Fetcher failure"

```bash
# Download failed - network issue or moved source
# Solution: Clean downloads and retry
rm -rf downloads/<package>
bitbake <package>
```

#### 3. "do_compile failed"

```bash
# Compilation error
# Solution: Check logs
cat tmp/work/armv8a-poky-linux/<package>/<version>/temp/log.do_compile

# Try building with verbose output
bitbake -v <package>
```

#### 4. "Disk full"

```bash
# Out of disk space
# Solution: Clean build artifacts
bitbake -c cleanall <large-package>
rm -rf tmp/work/*
# Or enable rm_work
echo 'INHERIT += "rm_work"' >> conf/local.conf
```

#### 5. "Sstate checksum mismatch"

```bash
# Sstate cache corruption
# Solution: Clear sstate
rm -rf sstate-cache/*
bitbake <package>
```

### Performance Issues

#### Slow Builds

```bash
# Enable parallel builds
BB_NUMBER_THREADS = "8"    # Increase
PARALLEL_MAKE = "-j 12"    # Increase (cores + 4)

# Enable rm_work to save disk I/O
INHERIT += "rm_work"

# Use fast storage (SSD) for build directory
```

#### High Memory Usage

```bash
# Reduce parallel builds
BB_NUMBER_THREADS = "4"    # Decrease
PARALLEL_MAKE = "-j 4"     # Decrease
```

### Runtime Issues

#### NPU Not Working

```bash
# On target:
# Check NPU driver
lsmod | grep galcore
dmesg | grep -i vsi

# Load NPU driver manually
modprobe galcore

# Check permissions
ls -l /dev/galcore
chmod 666 /dev/galcore  # Temporary fix

# Verify NPU delegate
find /usr/lib -name "*vsi*"
```

#### TensorFlow Lite Errors

```bash
# Check TensorFlow Lite installation
python3 -c "import tflite_runtime.interpreter as tflite; print(tflite.__version__)"

# Check VX delegate
python3 -c "from tflite_runtime.interpreter import load_delegate; print(load_delegate('libvx_delegate.so'))"
```

---

## Tips and Best Practices

### 1. Use Shared Downloads and Sstate

```bash
# In local.conf
DL_DIR = "/shared/yocto/downloads"
SSTATE_DIR = "/shared/yocto/sstate-cache"
```

### 2. Enable Parallel Execution

```bash
BB_NUMBER_THREADS = "${@oe.utils.cpu_count()}"
PARALLEL_MAKE = "-j ${@oe.utils.cpu_count() + 2}"
```

### 3. Monitor Build

```bash
# Real-time progress
watch -n 1 'bitbake-layers show-recipes | wc -l'

# Build statistics
buildstats-summary tmp/buildstats/
```

### 4. Incremental Development

```bash
# Use devtool for rapid iteration
devtool modify <package>
# Edit, build, deploy
devtool deploy-target <package> root@<target-ip>
```

### 5. Keep Logs

```bash
# Save build logs
mkdir -p ~/build-logs
cp -r tmp/log ~/build-logs/$(date +%Y%m%d-%H%M%S)/
```

---

**For integration with existing BSP, see [INTEGRATION.md](INTEGRATION.md).**
**For testing procedures, see [TESTING.md](TESTING.md).**
