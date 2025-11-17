# RADAE BitBake Recipes

This directory contains BitBake recipes for building RADAE (Radio Autoencoder) components for embedded Linux systems using the Yocto Project.

## Recipe Overview

### 1. radae-native_git.bb
**Purpose:** Native build for PyTorch training and model development tools

**Contents:**
- PyTorch training scripts
- Model export utilities
- Python radae module
- Weight exchange tools

**Use Case:** Build host tools for training models and exporting weights

**Installation:**
```bash
bitbake radae-native
```

### 2. radae_1.0.bb
**Purpose:** Main RADAE runtime package for target devices

**Contents:**
- Optimized C implementation with NEON/SIMD support
- FARGAN vocoder from Opus
- Core encoder/decoder libraries
- Runtime executables (radae_tx, radae_rx, lpcnet_demo)

**Installation:**
```bash
bitbake radae
```

### 3. radae-models_1.0.bb
**Purpose:** Pre-trained PyTorch model packages

**Sub-packages:**
- radae-models-production: Core model19_check3 only
- radae-models-reference: Development/test models

**Installation:**
```bash
bitbake radae-models
```

### 4. radae-tools_1.0.bb
**Purpose:** Development, testing, and evaluation tools

**Installation:**
```bash
bitbake radae-tools
```

## License

All recipes follow the BSD-2-Clause license from the upstream RADAE project.
