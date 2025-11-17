# meta-radae Yocto Layer Structure

## Created Files Summary

This document summarizes the complete Yocto layer structure created for RADAE implementation on i.MX 8M Plus.

## Directory Structure

```
meta-radae/
├── conf/                                    # Layer configuration
│   ├── layer.conf                          # Main layer configuration
│   ├── machine/
│   │   └── imx8mp-radae.conf              # i.MX 8M Plus machine config
│   └── distro/
│       └── radae-distro.conf              # RADAE distribution config
│
├── recipes-radae/                          # RADAE core recipes
│   └── README.md                          # Recipe documentation
│
├── recipes-ml/                             # Machine learning recipes
│   └── README.md                          # ML recipe documentation
│
├── recipes-support/                        # Supporting libraries
│   └── README.md                          # Support recipe documentation
│
├── recipes-kernel/                         # Kernel modifications
│   └── README.md                          # Kernel recipe documentation
│
├── recipes-core/                           # Core system recipes
│   └── README.md                          # Core recipe documentation
│
├── README.md                              # Layer overview and documentation
└── COPYING.MIT                            # License file
```

## Configuration Files

### 1. conf/layer.conf (49 lines)

**Purpose**: Main layer configuration file that defines:
- Layer dependencies: meta-freescale, meta-ml, meta-python
- Layer compatibility: Kirkstone, Langdale, Mickledore, Nanbield, Scarthgap
- Layer priority: 8
- RADAE-specific variables

**Key Variables**:
```bitbake
RADAE_VERSION = "1.0.0"
RADAE_NPU_SUPPORT = "1"
RADAE_BACKEND = "npu"
RADAE_MODEL_DIR = "/usr/share/radae/models"
RADAE_CONFIG_DIR = "/etc/radae"
RADAE_DATA_DIR = "/var/lib/radae"
```

### 2. conf/machine/imx8mp-radae.conf (107 lines)

**Purpose**: Machine configuration for i.MX 8M Plus with RADAE support

**Features**:
- NPU support with VeriSilicon driver
- GPU configuration (Vivante GC7000UL)
- TensorFlow Lite with VX delegate
- Audio support (ALSA)
- Wayland graphics backend
- U-Boot and kernel configuration
- Device tree setup

**Hardware Support**:
- Neural Processing Unit (NPU)
- GPU acceleration
- WiFi and Bluetooth
- Audio interfaces
- Serial console (115200 baud)

### 3. conf/distro/radae-distro.conf (158 lines)

**Purpose**: Distribution configuration optimized for RADAE deployment

**Features**:
- Systemd init manager
- Wayland graphics
- Machine learning support
- Python 3.10+ with scientific packages
- Real-time capabilities
- Security features (seccomp)

**Distro Features**:
```bitbake
DISTRO_FEATURES = "systemd wayland opengl pulseaudio wifi bluetooth ml npu tensorflow-lite"
```

**Optimization**:
- GCC optimization flags
- ARM NEON support
- FP16 precision
- NPU acceleration

## Recipe Directories

### recipes-radae/

**Purpose**: RADAE-specific application recipes

**Expected Contents**:
- radae-core: Main RADAE library
- radae-encoder: Encoding application
- radae-decoder: Decoding application
- radae-models: Pre-trained models
- radae-tools: Development utilities
- radae-examples: Example applications

**Documentation**: Comprehensive README with recipe templates and integration notes

### recipes-ml/

**Purpose**: Machine learning framework recipes

**Expected Contents**:
- tensorflow-lite: TensorFlow Lite runtime
- tflite-models: Optimized RADAE models
- vx-delegate: NPU acceleration delegate
- model-optimization: Quantization tools

**Documentation**: NPU integration guide, model optimization instructions, performance tuning

### recipes-support/

**Purpose**: Supporting libraries and utilities

**Expected Contents**:
- audio-libs: Audio processing (Codec2, libsamplerate)
- dsp-libs: DSP utilities (FFTW, liquid-dsp)
- radio-tools: Radio communication tools

**Documentation**: Library integration, NEON optimization, audio pipeline setup

### recipes-kernel/

**Purpose**: Linux kernel modifications

**Expected Contents**:
- linux-imx: Kernel patches and configs
- kernel-modules: Custom modules
- device-tree: DTS overlays
- firmware: NPU firmware

**Documentation**: Kernel configuration fragments, device tree customization, debugging guide

### recipes-core/

**Purpose**: Core system modifications

**Expected Contents**:
- images: Custom RADAE images
- systemd: Service files
- packagegroups: RADAE package groups
- init-scripts: Startup scripts

**Documentation**: Image recipes, systemd service examples, package groups

## Documentation Files

### README.md (9,782 bytes)

Comprehensive layer documentation including:
- Layer overview and dependencies
- Quick start guide
- Build instructions
- Hardware requirements
- Development workflow
- Performance tuning
- Troubleshooting
- Resource links

### COPYING.MIT (1,067 bytes)

MIT License for the layer

## Layer Configuration Summary

### Layer Metadata
```
Layer Name:        meta-radae
Version:           1.0
Priority:          8
Compatibility:     Kirkstone+ (LTS and later)
```

### Dependencies
```
- poky (core)
- meta-freescale (NXP BSP)
- meta-ml (eIQ ML framework)
- meta-python (Python support)
```

### Supported Platforms
```
Primary:  i.MX 8M Plus (imx8mp-radae)
Future:   Other i.MX 8 family processors
```

### Key Features
```
- NPU acceleration (VeriSilicon)
- TensorFlow Lite integration
- Real-time audio processing
- Systemd service management
- Wayland graphics
- Python 3 ML stack
```

## Build Integration

### Adding to Build

1. Clone layer to Yocto workspace:
```bash
git clone <repository> sources/meta-radae
```

2. Add to bblayers.conf:
```python
BBLAYERS += "${BSPDIR}/sources/meta-radae"
```

3. Configure local.conf:
```bash
MACHINE = "imx8mp-radae"
DISTRO = "radae-distro"
ACCEPT_FSL_EULA = "1"
```

4. Build:
```bash
bitbake radae-image-base
```

### Image Types

The layer supports building:
- **radae-image-minimal**: Minimal RADAE runtime
- **radae-image-base**: Base image with development tools
- **radae-image-full**: Full-featured development image

## Variable Reference

### RADAE Configuration Variables

| Variable | Default | Description |
|----------|---------|-------------|
| RADAE_VERSION | "1.0.0" | RADAE version |
| RADAE_NPU_SUPPORT | "1" | Enable NPU support |
| RADAE_BACKEND | "npu" | Inference backend (npu/cpu/gpu) |
| RADAE_OPTIMIZE_LEVEL | "2" | Optimization level (0-3) |
| RADAE_USE_NEON | "1" | Enable ARM NEON |
| RADAE_USE_FP16 | "1" | Use FP16 precision |
| RADAE_BATCH_SIZE | "1" | Inference batch size |
| RADAE_MODEL_DIR | "/usr/share/radae/models" | Model directory |
| RADAE_CONFIG_DIR | "/etc/radae" | Configuration directory |
| RADAE_DATA_DIR | "/var/lib/radae" | Data directory |

### NPU Configuration Variables

| Variable | Default | Description |
|----------|---------|-------------|
| IMX_NPU_ENABLE | "1" | Enable i.MX NPU |
| IMX_NPU_VERSION | "2.5.0" | NPU driver version |
| NPU_TOOLS_ENABLE | "1" | Include NPU tools |

## File Sizes

```
conf/layer.conf              : 1,336 bytes (49 lines)
conf/machine/imx8mp-radae.conf: 2,711 bytes (107 lines)
conf/distro/radae-distro.conf : 3,639 bytes (158 lines)
README.md                    : 9,782 bytes
COPYING.MIT                  : 1,067 bytes
recipes-radae/README.md      : 2,095 bytes
recipes-ml/README.md         : 2,297 bytes
recipes-support/README.md    : 2,005 bytes
recipes-kernel/README.md     : 3,796 bytes
recipes-core/README.md       : 5,502 bytes
```

## Next Steps

After setting up this layer structure:

1. **Create Recipe Files**: Implement BitBake recipes in each recipes-* directory
2. **Add Model Files**: Place TensorFlow Lite models in recipes-ml
3. **Configure Kernel**: Add kernel config fragments and device tree overlays
4. **Create Images**: Define custom image recipes in recipes-core/images
5. **Add Services**: Create systemd service files for RADAE applications
6. **Testing**: Build and test on i.MX 8M Plus hardware
7. **Documentation**: Update READMEs with actual recipe examples

## Maintenance

- Update LAYERSERIES_COMPAT for new Yocto releases
- Keep dependencies synchronized with upstream layers
- Test with each new Yocto LTS release
- Document any platform-specific quirks

## License

This layer is licensed under the MIT License. See COPYING.MIT for details.

Individual recipes may have different licenses as specified in their LICENSE variables.
