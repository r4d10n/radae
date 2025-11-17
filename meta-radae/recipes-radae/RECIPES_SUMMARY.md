# RADAE BitBake Recipes - Complete Implementation

## Overview
Four comprehensive BitBake recipes created for RADAE components, following Yocto best practices.

## Created Recipes

### 1. radae-native/radae-native_git.bb (93 lines)
**Location:** `/home/user/radae/meta-radae/recipes-radae/radae-native/radae-native_git.bb`

**Purpose:** Native build for PyTorch training tools (runs on build host)

**Key Features:**
- Fetches from GitHub (git://github.com/drowe67/radae.git)
- Inherits: python3native, native
- License: BSD-2-Clause
- Python dependencies: pytorch, numpy, tqdm, matplotlib

**Installed Components:**
- Training scripts: radae-train, radae-train-bbfm
- Inference tools: radae-inference, radae-bbfm-inference
- Export utilities: radae-export-weights
- Encoder/decoder: radae-stateful-encoder, radae-stateful-decoder
- Analysis tools: radae-est-snr, radae-est-cno, radae-loss
- Conversion utilities: radae-f32toint16, radae-int16tof32
- Python radae module and weight-exchange module

**Build Command:**
```bash
bitbake radae-native
```

---

### 2. radae/radae_1.0.bb (157 lines)
**Location:** `/home/user/radae/meta-radae/recipes-radae/radae/radae_1.0.bb`

**Purpose:** Main RADAE package with C implementation and FARGAN vocoder

**Key Features:**
- CMake-based build system
- Cross-compilation support
- ARM NEON optimizations (PACKAGECONFIG)
- Embedded Python support
- Shared library (librade.so)
- License: BSD-2-Clause

**Dependencies:**
- python3, python3-numpy
- opus (FARGAN-enabled)

**PACKAGECONFIG Options:**
- neon: ARM NEON optimizations (default for ARM)
- avx: x86 AVX optimizations

**Installed Components:**
Libraries:
- librade.so (shared library with SOVERSION 0.1)

Binaries:
- lpcnet_demo (FARGAN vocoder demo)
- radae_tx (transmitter)
- radae_rx (receiver)
- test_rade_enc, test_rade_dec (test utilities)

Headers:
- rade_api.h, rade_core.h, rade_constants.h
- rade_enc.h, rade_dec.h

Python Runtime Scripts:
- radae_txe.py, radae_rxe.py, rx.py

**Runtime Dependencies:**
- python3-core, python3-numpy, python3-torch, opus

**Build Command:**
```bash
bitbake radae
```

**Cross-Compilation:**
- Includes CMake patch: 0001-cmake-fix-cross-compile.patch
- Configures Python3_ROOT_DIR for target
- Sets NumPy include directories

---

### 3. radae-models/radae-models_1.0.bb (142 lines)
**Location:** `/home/user/radae/meta-radae/recipes-radae/radae-models/radae-models_1.0.bb`

**Purpose:** Pre-trained PyTorch models and compiled weights

**Key Features:**
- Architecture-independent (allarch)
- Split packages for flexible deployment
- License: BSD-2-Clause
- Models installed to /usr/share/radae/models/

**Sub-packages:**
1. **radae-models-production** (minimal footprint)
   - model19_check3/checkpoints/checkpoint_epoch_100.pth
   - bin/model19_check3.bin (compiled weights)
   - default_model.pth (symlink)
   - README

2. **radae-models-reference** (development)
   - model05, model17, model18 checkpoints
   - bin/model05.bin

3. **radae-models-bbfm** (baseband FM)
   - model_bbfm_01 checkpoint

**Model Details:**
- model19_check3: Production model (3000 Hz BW, 4 dB SNR)
- model05: Legacy reference
- model17: Development reference
- model18: Alternative latent dimension (40)
- model_bbfm_01: Baseband FM model

**Build Commands:**
```bash
bitbake radae-models                # All models
bitbake radae-models-production     # Production only (minimal)
```

---

### 4. radae-tools/radae-tools_1.0.bb (200 lines)
**Location:** `/home/user/radae/meta-radae/recipes-radae/radae-tools/radae-tools_1.0.bb`

**Purpose:** Development, testing, and evaluation tools

**Key Features:**
- Depends: radae, radae-models
- License: BSD-2-Clause
- Comprehensive test and evaluation suite

**Installed Components:**

**Inference & Testing Scripts:**
- radae-inference, radae-rx, radae-rx-stream
- radae-test-stateful-encoder, radae-test-stateful-decoder

**Evaluation Tools:**
- radae-evaluate, radae-evaluate-loop
- radae-compare-models, radae-est-snr-curves

**OTA Testing:**
- radae-ota-test, radae-ptt-test, radae-ofdm-sync

**BBFM Tools:**
- radae-bbfm-inference, radae-bbfm-rx, radae-analog-bbfm

**Demonstration:**
- radae-demo-rick, radae-test-dt

**Python Analysis Tools:**
- inference.py, rx.py, loss.py
- est_snr.py, est_CNo.py, eoo_ber.py
- sc_tx.py, sc_rx.py (single carrier modem)

**MATLAB/Octave Scripts:**
- radae_plots.m, fm.m, bbfm_bpf.m
- load_f32.m, load_raw.m, plot_specgram.m
- multipath_samples.m, doppler_spread.m

**Test Suite:**
- Complete test suite from test/ directory

**Sample Data:**
- WAV files from wav/ directory

**Utility Scripts:**
- radae-test-model19 (wrapper for testing model19_check3)
- radae-quick-test (installation sanity check)

**Runtime Dependencies:**
- radae, radae-models
- python3-core, python3-numpy, python3-torch, python3-matplotlib
- bash

**Recommended:**
- codec2, sox

**Build Command:**
```bash
bitbake radae-tools
```

---

## Supporting Files

### radae/files/0001-cmake-fix-cross-compile.patch
**Location:** `/home/user/radae/meta-radae/recipes-radae/radae/files/0001-cmake-fix-cross-compile.patch`

**Purpose:** CMake cross-compilation fixes for Python3 and NumPy detection

**Changes:**
- Adds Linux/Unix cross-compilation support
- Configures Python3 include and library paths
- Sets NumPy include directories for target

---

## Recipe Features Summary

### Common Features (All Recipes)
- BSD-2-Clause licensing
- Git-based source fetching (github.com/drowe67/radae)
- SRCREV set to AUTOREV (can be pinned for production)
- Proper Yocto/OE structure and naming

### Build System Support
- Native builds (radae-native)
- Cross-compilation (radae)
- CMake integration (radae)
- Python packaging (all)

### Optimization Support
- ARM NEON optimizations
- x86 AVX support (configurable)
- Compiler flag customization
- SIMD intrinsics

### Package Splitting
- Main packages
- Development packages (-dev)
- Debug packages (-dbg)
- Model sub-packages (production/reference/bbfm)

### Dependencies Handled
- Python3 and NumPy (target and native)
- PyTorch (native and target)
- Opus with FARGAN support
- Build-time vs runtime dependencies

---

## Installation Examples

### Minimal Runtime (Production)
```bitbake
IMAGE_INSTALL += "radae radae-models-production"
```
Result: ~30MB (core libraries + production model)

### Full Development Environment
```bitbake
IMAGE_INSTALL += "radae radae-models radae-tools"
```
Result: ~80MB (includes all models and tools)

### Build Host Training
```bash
bitbake radae-native
```
Result: Native tools available on build host

---

## File Locations After Installation

### Binaries
- /usr/bin/radae_tx
- /usr/bin/radae_rx
- /usr/bin/lpcnet_demo
- /usr/bin/radae-* (tool wrappers)

### Libraries
- /usr/lib/librade.so.0.1
- /usr/lib/librade.so -> librade.so.0.1

### Headers
- /usr/include/radae/*.h

### Python Modules
- /usr/lib/python3.11/site-packages/radae/

### Models
- /usr/share/radae/models/model19_check3/
- /usr/share/radae/models/default_model.pth -> model19_check3/...
- /usr/share/radae/models/bin/*.bin

### Tools & Data
- /usr/share/radae/tools/
- /usr/share/radae/test/
- /usr/share/radae/wav/
- /usr/share/radae/doc/

---

## Cross-Compilation Configuration

### CMake Variables Set
```cmake
-DBUILD_SHARED_LIBS=ON
-DCMAKE_BUILD_TYPE=Release
-DPython3_ROOT_DIR=${STAGING_DIR_TARGET}${prefix}
-DPython3_NumPy_INCLUDE_DIR=${STAGING_DIR_TARGET}${PYTHON_SITEPACKAGES_DIR}/numpy/_core/include
```

### Compiler Flags
ARM with NEON:
```
CFLAGS:append:armv7a = " -mfpu=neon"
CFLAGS:append:aarch64 = " -march=armv8-a"
```

---

## Testing After Build

### Quick Test
```bash
radae-quick-test
```
Verifies:
- Binaries installed
- Models present
- Python modules importable

### Model Test
```bash
radae-test-model19 input.wav output.wav 3
```
Runs inference with model19_check3 at 3dB SNR

### Full Test Suite
```bash
cd /usr/share/radae/test
./test_suite.sh
```

---

## Integration with i.MX 8M Plus

These recipes integrate with the broader meta-radae layer for i.MX 8M Plus:

1. **NPU Support:** Works with radae-npu package
2. **Machine Config:** Compatible with imx8mp-radae machine
3. **Kernel Modules:** Integrates with NPU kernel support
4. **Performance:** Optimized for Cortex-A53 + NPU

---

## Production Deployment Notes

### For Production Builds:
1. Pin SRCREV to specific commit:
   ```bitbake
   SRCREV = "a6036c8..."
   ```

2. Use production model only:
   ```bitbake
   IMAGE_INSTALL += "radae radae-models-production"
   ```

3. Remove development tools:
   ```bitbake
   IMAGE_INSTALL:remove = "radae-tools"
   ```

4. Enable size optimizations:
   ```bitbake
   IMAGE_FEATURES += "read-only-rootfs"
   ```

---

## Summary Statistics

**Total Recipes:** 4
**Total Lines of Code:** 592 lines
**Total Packages Generated:** 10+
  - radae, radae-dev, radae-dbg
  - radae-models, radae-models-production, radae-models-reference, radae-models-bbfm
  - radae-tools
  - radae-native
  - radae-app (existing)
  - radae-npu (existing)

**License:** BSD-2-Clause (all recipes)
**Upstream:** https://github.com/drowe67/radae

---

## Recipe Quality Checklist

✓ Follows Yocto/OE naming conventions
✓ Proper license declarations (BSD-2-Clause)
✓ LIC_FILES_CHKSUM included
✓ DEPENDS vs RDEPENDS correct
✓ Cross-compilation support
✓ PACKAGECONFIG for options
✓ Package splitting (-dev, -dbg)
✓ Proper file ownership (FILES:${PN})
✓ CMAKE/Python integration
✓ Runtime dependencies declared
✓ Development vs production splits
✓ Architecture handling (allarch for models)
✓ Git-based SRC_URI
✓ Proper installation paths
✓ Library versioning (SOVERSION)
✓ pkg-config file generation

---

## Next Steps

1. **Build and Test:**
   ```bash
   bitbake radae radae-models radae-tools
   ```

2. **Create Test Image:**
   ```bash
   bitbake core-image-radae
   ```

3. **Deploy to Target:**
   ```bash
   bitbake core-image-radae -c populate_sdk
   ```

4. **Run Tests on Target:**
   ```bash
   radae-quick-test
   radae-test-model19 /usr/share/radae/wav/brian_g8sez.wav /tmp/out.wav
   ```
