# RADAE NPU Integration - Files Created

This document summarizes all files created for NPU integration in the meta-radae layer.

**Created:** 2025-11-17
**Purpose:** NPU-accelerated RADAE implementation for i.MX 8M Plus

## Directory Structure

```
meta-radae/
├── conf/
│   ├── layer.conf                       # Layer configuration
│   └── local.conf.sample                # Sample build configuration
├── recipes-radae/
│   ├── radae-npu/                       # Main NPU library
│   │   ├── radae-npu_1.0.bb            # BitBake recipe
│   │   └── files/
│   │       ├── CMakeLists.txt          # CMake build configuration
│   │       ├── npu_encoder.cpp         # NPU encoder implementation
│   │       ├── npu_decoder.cpp         # NPU decoder implementation
│   │       ├── radae_streaming.cpp     # Streaming/NEON optimizations
│   │       ├── radae_npu.h             # C API header
│   │       └── npu_wrapper.h           # C++ wrapper header
│   └── radae-npu-test/                  # Test utilities
│       ├── radae-npu-test_1.0.bb       # BitBake recipe
│       └── files/
│           ├── CMakeLists.txt          # CMake build configuration
│           ├── npu_benchmark.cpp       # Performance benchmarking
│           ├── npu_accuracy_test.cpp   # Accuracy verification
│           ├── npu_profiler.cpp        # Performance profiling
│           ├── test_utils.h            # Test utilities header
│           ├── test_utils.cpp          # Test utilities implementation
│           ├── test_data.h             # Test data definitions
│           └── test_vectors/
│               ├── generate_test_data.py  # Test vector generator
│               ├── speech_8k.raw          # Sample audio (8kHz, 16-bit)
│               └── expected_features.bin  # Expected feature vectors
└── README.md                            # Already existed (comprehensive guide)
```

## File Descriptions

### 1. BitBake Recipes

#### `/recipes-radae/radae-npu/radae-npu_1.0.bb`
- **Purpose:** Build NPU-accelerated RADAE library
- **Dependencies:** tensorflow-lite, imx-vsi-npu
- **Outputs:**
  - `/usr/lib/libradae-npu.so` - Shared library
  - `/usr/include/radae/*.h` - API headers
  - `/etc/radae/npu.conf` - Runtime configuration
- **Features:**
  - NPU detection and auto-fallback to CPU
  - INT8 quantization support
  - NEON optimizations for ARM Cortex-A53
  - Runtime configuration via PACKAGECONFIG

#### `/recipes-radae/radae-npu-test/radae-npu-test_1.0.bb`
- **Purpose:** Build testing and benchmarking utilities
- **Dependencies:** radae-npu, tensorflow-lite
- **Outputs:**
  - `/usr/bin/radae-npu-benchmark` - Performance benchmarking tool
  - `/usr/bin/radae-npu-accuracy` - Accuracy verification tool
  - `/usr/bin/radae-npu-profiler` - Performance profiling tool
  - `/usr/bin/radae-npu-quick-test` - Quick functionality test script
  - `/usr/bin/radae-npu-stress-test` - Stress testing script
  - `/usr/share/radae/test-vectors/*` - Test data files

### 2. Source Files - Main Library

#### `/recipes-radae/radae-npu/files/radae_npu.h`
- **Lines:** 148
- **Purpose:** C API for NPU-accelerated RADAE
- **Key Components:**
  - Encoder/decoder handle types
  - Configuration structures
  - Error codes and quantization modes
  - Performance statistics
  - Audio processing constants (8kHz, 160 samples/frame, 20ms)

#### `/recipes-radae/radae-npu/files/npu_wrapper.h`
- **Lines:** 155
- **Purpose:** C++ wrapper for TensorFlow Lite NPU integration
- **Key Components:**
  - NPU delegate configuration
  - TensorFlow Lite interface
  - NEON preprocessing utilities
  - Streaming buffer management
  - Quantization utilities

#### `/recipes-radae/radae-npu/files/npu_encoder.cpp`
- **Lines:** 186
- **Purpose:** NPU encoder implementation
- **Key Features:**
  - Pre-emphasis filtering
  - NEON-accelerated audio preprocessing
  - TensorFlow Lite inference
  - Performance profiling
  - Batch processing support

#### `/recipes-radae/radae-npu/files/npu_decoder.cpp`
- **Lines:** 168
- **Purpose:** NPU decoder implementation
- **Key Features:**
  - De-emphasis filtering
  - NEON-accelerated audio postprocessing
  - TensorFlow Lite inference
  - Performance profiling
  - SNR computation

#### `/recipes-radae/radae-npu/files/radae_streaming.cpp`
- **Lines:** 308
- **Purpose:** NPU wrapper and NEON optimizations
- **Key Components:**
  - NPU delegate attachment
  - VeriSilicon library integration
  - NEON int16↔float conversion
  - Pre/de-emphasis filters
  - Quantization utilities
  - Streaming buffer management

#### `/recipes-radae/radae-npu/files/CMakeLists.txt`
- **Lines:** 74
- **Purpose:** CMake build configuration
- **Features:**
  - C++17 standard
  - NPU delegate linking
  - NEON optimization flags
  - PACKAGECONFIG options
  - Cross-compilation support

### 3. Source Files - Test Utilities

#### `/recipes-radae/radae-npu-test/files/test_utils.h`
- **Lines:** 85
- **Purpose:** Test utilities header
- **Components:**
  - TestSuite class
  - Timer class
  - ProgressBar class
  - Statistics tracker
  - Signal generation/analysis functions

#### `/recipes-radae/radae-npu-test/files/test_utils.cpp`
- **Lines:** 225
- **Purpose:** Test utilities implementation
- **Functions:**
  - File I/O (audio/features)
  - Signal generation (sine, noise, sweep)
  - Signal analysis (PESQ approximation, RMSE, correlation)
  - Progress tracking
  - Statistical analysis

#### `/recipes-radae/radae-npu-test/files/test_data.h`
- **Lines:** 95
- **Purpose:** Test data and configuration
- **Components:**
  - Test vector paths
  - Accuracy thresholds
  - Performance benchmarks
  - Test configuration structure

#### `/recipes-radae/radae-npu-test/files/npu_benchmark.cpp`
- **Lines:** 217
- **Purpose:** Performance benchmarking tool
- **Features:**
  - Encoder/decoder benchmarking
  - Real-time factor calculation
  - NPU vs CPU comparison
  - Configurable iterations
  - Performance statistics

#### `/recipes-radae/radae-npu-test/files/npu_accuracy_test.cpp`
- **Lines:** 285
- **Purpose:** Accuracy verification tool
- **Tests:**
  - Encoder/decoder creation
  - Signal processing accuracy
  - Batch processing
  - Round-trip SNR verification
  - Output validation

#### `/recipes-radae/radae-npu-test/files/npu_profiler.cpp`
- **Lines:** 241
- **Purpose:** Performance profiling tool
- **Features:**
  - Long-duration testing
  - Real-time monitoring
  - NPU utilization tracking
  - CSV logging
  - Stress testing mode

#### `/recipes-radae/radae-npu-test/files/CMakeLists.txt`
- **Lines:** 46
- **Purpose:** Test suite build configuration
- **Outputs:**
  - radae-npu-benchmark
  - radae-npu-accuracy
  - radae-npu-profiler

### 4. Test Data

#### `/recipes-radae/radae-npu-test/files/test_vectors/generate_test_data.py`
- **Lines:** 42
- **Purpose:** Generate test vector data
- **Outputs:**
  - `speech_8k.raw` - 1 second of 440Hz sine wave at 8kHz (binary)
  - `expected_features.bin` - 50 frames × 20 features (binary)

### 5. Configuration Files

#### `/conf/layer.conf`
- **Purpose:** Yocto layer configuration
- **Features:**
  - Layer dependencies (meta-freescale, meta-ml, etc.)
  - Layer compatibility (kirkstone, langdale, mickledore, etc.)
  - BBFILES pattern matching
  - Preferred versions

#### `/conf/local.conf.sample`
- **Lines:** 242
- **Purpose:** Sample Yocto build configuration
- **Sections:**
  - Machine selection (imx8mpevk)
  - NPU/GPU/VPU enablement
  - RADAE package configuration
  - Build performance tuning
  - Development tools
  - Security settings

## API Summary

### C API (radae_npu.h)

**Configuration:**
```c
radae_config_t config;
radae_get_default_config(&config);
config.exec_mode = RADAE_EXEC_AUTO;  // NPU with CPU fallback
config.quantization = RADAE_QUANT_INT8;
config.num_threads = 4;
```

**Encoder:**
```c
radae_encoder_t encoder = radae_encoder_create(&config, &error);
radae_encoder_process(encoder, audio_in, features_out);
radae_encoder_get_stats(encoder, &stats);
radae_encoder_destroy(encoder);
```

**Decoder:**
```c
radae_decoder_t decoder = radae_decoder_create(&config, &error);
radae_decoder_process(decoder, features_in, audio_out);
radae_decoder_get_stats(decoder, &stats);
radae_decoder_destroy(decoder);
```

## Performance Targets (i.MX 8M Plus @ 1.8GHz)

| Component | NPU Target | CPU Fallback | Status |
|-----------|-----------|--------------|--------|
| Encoder   | < 2 ms    | < 10 ms      | Target |
| Decoder   | < 2 ms    | < 10 ms      | Target |
| RT Factor | < 0.1     | < 0.5        | Target |

## Accuracy Targets

| Metric | Threshold | Purpose |
|--------|-----------|---------|
| Feature MSE | < 0.01 | Feature extraction accuracy |
| Audio SNR | > 20 dB | Reconstruction quality |
| Correlation | > 0.95 | Signal fidelity |

## Build Instructions

### Quick Build

```bash
# Add layer
bitbake-layers add-layer meta-radae

# Build NPU library
bitbake radae-npu

# Build test utilities
bitbake radae-npu-test

# Build both
bitbake radae-npu radae-npu-test
```

### Integration with Image

```bash
# In conf/local.conf
IMAGE_INSTALL:append = " radae-npu radae-npu-test"

# Build image
bitbake core-image-minimal
```

## Testing on Target

### Quick Test
```bash
radae-npu-quick-test
```

### Benchmark
```bash
radae-npu-benchmark --quick    # 10 iterations
radae-npu-benchmark --full     # 1000 iterations
```

### Accuracy Test
```bash
radae-npu-accuracy --test-vectors /usr/share/radae/test-vectors
```

### Profiling
```bash
radae-npu-profiler --duration 60 --log /tmp/profile.csv
```

### Stress Test
```bash
radae-npu-stress-test 300  # 5 minutes
```

## Dependencies

### Build-time Dependencies
- `tensorflow-lite` - TensorFlow Lite runtime
- `imx-vsi-npu` - VeriSilicon NPU delegate
- `cmake` - Build system (>= 3.10)
- `pkgconfig` - Package configuration

### Runtime Dependencies
- `tensorflow-lite` - Inference runtime
- `imx-vsi-npu` - NPU delegate library
- `firmware-imx-vpu-imx8` - NPU firmware
- `imx-vpu-hantro` - NPU driver

### Optional Dependencies
- `opencv` - For debugging/visualization

## File Statistics

### Total Files Created (This Session)
- **BitBake Recipes:** 2
- **C/C++ Headers:** 4
- **C/C++ Source:** 7
- **CMake Files:** 2
- **Test Scripts:** 1
- **Configuration Files:** 2
- **Test Data Files:** 2
- **Total Lines of Code:** ~2,500

### Size Breakdown
- **Recipe Files:** ~150 lines
- **C/C++ Code:** ~2,000 lines
- **CMake Files:** ~120 lines
- **Test Scripts:** ~50 lines
- **Configuration:** ~280 lines

## Integration Points

### With Existing RADAE Code
- Uses standard RADAE audio parameters (8kHz, 160 samples/frame)
- Compatible with existing feature dimension (20)
- Maintains latent dimension (80)

### With TensorFlow Lite
- Uses TFLite C++ API
- Supports VeriSilicon NPU delegate
- Auto-detection of NPU availability
- Graceful CPU fallback

### With Yocto/OpenEmbedded
- Standard BitBake recipe format
- PACKAGECONFIG for feature selection
- Machine feature detection (MACHINE_FEATURES)
- Systemd integration ready

## Next Steps

1. **Model Conversion:** Convert PyTorch RADAE models to TFLite INT8
2. **Hardware Testing:** Test on actual i.MX 8M Plus hardware
3. **Performance Tuning:** Optimize NPU delegate settings
4. **Integration:** Connect with RADAE DSP/OFDM components
5. **Documentation:** Add application notes and examples

## References

- **RADAE Project:** https://github.com/drowe67/radae
- **TensorFlow Lite:** https://www.tensorflow.org/lite
- **i.MX 8M Plus:** https://www.nxp.com/imx8mplus
- **Yocto Project:** https://www.yoctoproject.org
- **meta-freescale:** https://github.com/Freescale/meta-freescale

## License

All files are released under the **MIT License**.

## Author

Created as part of the RADAE i.MX 8M Plus integration effort.
