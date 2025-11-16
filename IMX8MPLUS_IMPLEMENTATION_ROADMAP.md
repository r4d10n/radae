# RADAE Implementation Roadmap for NXP i.MX 8M Plus

**Platform:** NXP i.MX 8M Plus
**Target Performance:** 8-10× real-time
**Power Budget:** 3-5W
**Date:** 2025-11-16

---

## Executive Summary

This document provides a complete roadmap for implementing RADAE on the NXP i.MX 8M Plus platform, leveraging its 2.3 TOPS NPU for neural network acceleration. The implementation follows a phased approach:

1. **Phase 1:** CPU-only baseline (1-2 weeks)
2. **Phase 2:** NPU model conversion (1-2 weeks)
3. **Phase 3:** NPU integration and optimization (2-3 weeks)
4. **Phase 4:** Full system integration and testing (1-2 weeks)

**Expected Results:**
- **Performance:** 8-10× real-time processing
- **Power:** 3-5W total system (vs 5-8W CPU-only)
- **Latency:** ~185ms end-to-end (120ms algorithmic + 65ms processing)
- **NPU Acceleration:** RADAE encoder/decoder, FARGAN vocoder

---

## Table of Contents

1. [Platform Overview](#1-platform-overview)
2. [Development Environment Setup](#2-development-environment-setup)
3. [Phase 1: CPU-Only Baseline](#phase-1-cpu-only-baseline)
4. [Phase 2: NPU Model Conversion](#phase-2-npu-model-conversion)
5. [Phase 3: NPU Integration](#phase-3-npu-integration)
6. [Phase 4: System Integration](#phase-4-system-integration)
7. [Performance Optimization](#7-performance-optimization)
8. [Testing and Validation](#8-testing-and-validation)
9. [Deployment](#9-deployment)

---

## 1. Platform Overview

### 1.1 i.MX 8M Plus Hardware Specifications

**Processing System:**
- **CPU:** Quad-core ARM Cortex-A53 @ 1.8 GHz
  - ARMv8-A architecture (64-bit)
  - NEON SIMD extensions (128-bit vector operations)
  - 32 KB L1 I-cache + 32 KB L1 D-cache per core
  - 512 KB L2 cache shared
- **GPU:** Vivante GC7000UL (optional for UI)
- **NPU:** 2.3 TOPS INT8 performance
  - Supports TensorFlow Lite, ONNX Runtime
  - INT8/INT16 quantization
  - Zero-copy memory interface

**Memory:**
- **RAM:** 2-4 GB LPDDR4
- **Storage:** eMMC 5.1 or SD card

**Peripherals:**
- **Audio:** I2S, SAI for codec interface
- **Network:** Gigabit Ethernet, optional WiFi
- **GPIO:** For PTT control

### 1.2 NPU Architecture (Neural Processing Unit)

**NXP Neural Processing Unit Details:**
- **Vendor:** Supplied by VeriSilicon
- **Architecture:** Fixed-function accelerator for common NN ops
- **Supported Operations:**
  - Fully connected (Dense) layers ✓
  - Convolution 1D/2D ✓
  - GRU/LSTM recurrent layers ✓
  - Activation functions (ReLU, tanh, sigmoid) ✓
  - Element-wise operations ✓
  - Batch normalization ✓
- **Data Types:** INT8, INT16, FP16 (int8 optimal for TOPS rating)
- **Memory:** Dedicated SRAM for weights/activations

**NPU Performance Characteristics:**
- **Peak:** 2.3 TOPS @ INT8 (2300 GOP/s)
- **Practical:** ~1.5-2.0 TOPS sustained (65-85% efficiency)
- **Latency:** 1-3ms invocation overhead
- **Power:** ~0.5-1.0W @ 100% utilization

### 1.3 RADAE Workload Mapping

**Component Distribution:**

| Component | Target | MMACs | NPU Speedup | Effective MMACs |
|-----------|--------|-------|-------------|-----------------|
| RADAE Encoder | NPU | 80 | 8-10× | 8-10 |
| RADAE Decoder | NPU | 80 | 8-10× | 8-10 |
| FARGAN Vocoder | NPU | 300 | 5-8× | 40-60 |
| Feature Extraction | CPU | 25 | 1× | 25 |
| OFDM Tx/Rx | CPU | <1 | N/A | <1 |
| Sync/Tracking | CPU | ~5 | N/A | ~5 |
| **Total** | **Mixed** | **~490** | **~6-8×** | **~98-109** |

**Expected Performance:**
- **Total effective compute:** ~100 MMACs (vs 490 MMACs CPU-only)
- **Real-time factor:** 8-10× (490 / 100 = 4.9× speedup, base 2× on A53)
- **Power reduction:** 60-75% for ML workload

---

## 2. Development Environment Setup

### 2.1 Hardware Requirements

**Development Board:**
- **NXP i.MX 8M Plus EVK** ($249) or
- **Variscite VAR-SOM-MX8M-PLUS** ($150-200) or
- **Boundary Devices Nitrogen8M Plus** ($200)

**Peripherals:**
- USB audio adapter (for initial testing)
- Serial console (USB-UART adapter)
- Ethernet cable
- 12V power supply (2-3A)
- SD card (32+ GB, Class 10)

**Host Development PC:**
- Ubuntu 20.04/22.04 LTS
- 16+ GB RAM
- 50+ GB free storage
- Cross-compilation toolchain

### 2.2 Software Stack

**Base Operating System:**
```bash
# Yocto-based Linux BSP
NXP L5.15.52_2.1.0 BSP (or later)
Kernel: 5.15.x
U-Boot: 2022.04
```

**Alternative: Debian/Ubuntu ARM64:**
```bash
# For easier development
Ubuntu 22.04 ARM64 for i.MX8
- Pre-built by NXP or community
- Simpler package management
```

### 2.3 Cross-Compilation Toolchain

**Install ARM64 GCC:**
```bash
# On host PC (Ubuntu x86_64)
sudo apt update
sudo apt install -y \
    gcc-aarch64-linux-gnu \
    g++-aarch64-linux-gnu \
    crossbuild-essential-arm64 \
    qemu-user-static

# Verify installation
aarch64-linux-gnu-gcc --version
```

**CMake Toolchain File:**
```cmake
# toolchain-imx8mp.cmake
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

set(CMAKE_FIND_ROOT_PATH /opt/fsl-imx-xwayland/5.15-kirkstone/sysroots/armv8a-poky-linux)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# NEON optimization flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv8-a+simd -mtune=cortex-a53")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv8-a+simd -mtune=cortex-a53")
```

### 2.4 NPU Software Stack

**eIQ Machine Learning Software:**
```bash
# NXP eIQ Toolkit includes:
# - TensorFlow Lite 2.x
# - ONNX Runtime
# - NPU delegates for inference
# - Model conversion tools

# Installation on i.MX 8M Plus
sudo apt install -y \
    python3-pip \
    python3-dev \
    libjpeg-dev \
    zlib1g-dev

# Install TensorFlow Lite Runtime with NPU delegate
pip3 install tflite-runtime
pip3 install tensorflow  # For model conversion
```

**NPU Delegate Library:**
```bash
# VeriSilicon NPU driver and delegate
# Installed via BSP or from NXP eIQ package

# Verify NPU availability
cat /sys/class/misc/galcore/device/driver/version
# Expected: GC kernel version: 6.4.x

# Check NPU device
ls -l /dev/galcore
# Expected: crw-rw---- 1 root video
```

### 2.5 Python Environment for Model Conversion

**On Development PC:**
```bash
# Create virtual environment
python3 -m venv radae-imx8mp
source radae-imx8mp/bin/activate

# Install PyTorch (for RADAE model loading)
pip install torch torchvision torchaudio --index-url https://download.pytorch.org/whl/cpu

# Install TensorFlow for conversion
pip install tensorflow==2.13.0

# Install ONNX tools
pip install onnx onnx-simplifier onnxruntime

# Install quantization tools
pip install tensorflow-model-optimization

# Clone RADAE repository
git clone https://github.com/drowe67/radae.git
cd radae
```

---

## Phase 1: CPU-Only Baseline

**Duration:** 1-2 weeks
**Goal:** Establish baseline performance with NEON-optimized CPU implementation

### 3.1 RADAE Compilation for ARM64

**Step 1: Build FARGAN Vocoder**
```bash
# On i.MX 8M Plus or cross-compile
cd radae
mkdir build-imx8mp
cd build-imx8mp

# Configure with NEON optimizations
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_FLAGS="-O3 -march=armv8-a+simd -mtune=cortex-a53" \
    -DCMAKE_CXX_FLAGS="-O3 -march=armv8-a+simd -mtune=cortex-a53"

# Build
make -j4

# Test lpcnet_demo (FARGAN vocoder)
./src/lpcnet_demo --help
```

**Step 2: Python RADAE Setup**
```bash
# Install Python dependencies on i.MX 8M Plus
pip3 install torch --index-url https://download.pytorch.org/whl/cpu
pip3 install numpy scipy matplotlib tqdm

# Download pre-trained model
cd ~/radae
wget https://github.com/drowe67/radae/releases/download/v0.1/model19_check3.tar.gz
tar xzf model19_check3.tar.gz

# Verify model loading
python3 -c "
import torch
model = torch.load('model19_check3/checkpoints/checkpoint_epoch_100.pth',
                   map_location='cpu')
print('Model loaded successfully')
print('Keys:', model.keys())
"
```

**Step 3: Baseline Performance Test**
```bash
# Run inference test
./inference.sh model19_check3/checkpoints/checkpoint_epoch_100.pth \
    wav/brian_g8sez.wav out_cpu.wav --EbNodB 100

# Measure CPU usage and timing
time python3 inference.py \
    --model-path model19_check3/checkpoints/checkpoint_epoch_100.pth \
    --input wav/brian_g8sez.wav \
    --output out_cpu.wav \
    --write-latent z.f32

# Expected on i.MX 8M Plus A53 @ 1.8 GHz:
# - Processing time: ~5-7 seconds for 10 second audio
# - Real-time factor: 1.4-2.0×
# - CPU usage: 80-95% (single core) or 25-30% (all cores)
```

### 3.2 NEON Optimization (C Implementation)

**NEON-Optimized GRU Layer:**
```c
// src/gru_neon.c
#include <arm_neon.h>

// GRU matrix-vector multiply with NEON intrinsics
void gru_dense_neon(
    float *output,          // [out_size]
    const float *input,     // [in_size]
    const float *weights,   // [out_size][in_size]
    const float *bias,      // [out_size]
    int in_size,
    int out_size)
{
    for (int i = 0; i < out_size; i++) {
        float32x4_t sum_vec = vdupq_n_f32(0.0f);

        // Process 4 elements at a time
        int j = 0;
        for (; j <= in_size - 4; j += 4) {
            float32x4_t w = vld1q_f32(&weights[i * in_size + j]);
            float32x4_t x = vld1q_f32(&input[j]);
            sum_vec = vmlaq_f32(sum_vec, w, x);  // Fused multiply-add
        }

        // Horizontal sum
        float sum = vaddvq_f32(sum_vec);  // ARMv8 instruction

        // Handle remaining elements
        for (; j < in_size; j++) {
            sum += weights[i * in_size + j] * input[j];
        }

        // Add bias
        output[i] = sum + bias[i];
    }
}

// GRU gate activation (sigmoid)
void gru_sigmoid_neon(float *x, int size) {
    for (int i = 0; i < size; i += 4) {
        float32x4_t v = vld1q_f32(&x[i]);

        // Approximate sigmoid: 0.5 + 0.5 * tanh(0.5 * x)
        // Using fast tanh approximation
        float32x4_t half = vdupq_n_f32(0.5f);
        v = vmulq_f32(v, half);

        // Fast tanh via lookup table or polynomial approximation
        // (simplified here, use proper implementation)
        v = vtanhq_f32(v);  // Custom tanh implementation

        v = vmlaq_f32(half, v, half);  // 0.5 + 0.5 * tanh_result
        vst1q_f32(&x[i], v);
    }
}
```

**Build with NEON:**
```cmake
# CMakeLists.txt additions
option(ENABLE_NEON "Enable ARM NEON optimizations" ON)

if(ENABLE_NEON)
    add_compile_options(-mfpu=neon)  # ARMv7
    # Or for ARMv8: -march=armv8-a+simd (already in flags)
    add_definitions(-DUSE_NEON)
endif()

add_library(radae_neon STATIC
    src/gru_neon.c
    src/conv1d_neon.c
)
```

### 3.3 Baseline Performance Profiling

**CPU Profiling with perf:**
```bash
# Install perf tools
sudo apt install linux-tools-generic

# Profile inference
sudo perf record -g python3 inference.py \
    --model-path model19_check3/checkpoints/checkpoint_epoch_100.pth \
    --input wav/brian_g8sez.wav \
    --output out.wav

# Analyze results
sudo perf report

# Expected hotspots:
# - GRU forward: 40-50%
# - Conv1D forward: 20-30%
# - FARGAN synthesis: 30-40%
```

**Power Measurement:**
```bash
# Monitor CPU frequency and power
watch -n 1 'cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_cur_freq'

# Use i.MX 8M Plus power monitoring (if available)
# Or external power meter on 12V input

# Expected CPU-only power:
# - Idle: 1-2W
# - RADAE inference: 5-8W
# - Per-core: ~1.2-1.5W @ 100%
```

---

## Phase 2: NPU Model Conversion

**Duration:** 1-2 weeks
**Goal:** Convert PyTorch RADAE models to TensorFlow Lite with INT8 quantization

### 4.1 Model Conversion Strategy

**Conversion Pipeline:**
```
PyTorch (.pth)
  → ONNX (.onnx)
  → TensorFlow (.pb)
  → TensorFlow Lite (.tflite)
  → Quantized TFLite (.tflite with INT8)
  → NPU-optimized (.tflite with NPU delegate)
```

### 4.2 PyTorch to ONNX Conversion

**Extract RADAE Encoder:**
```python
# export_encoder_onnx.py
import torch
import torch.onnx
import sys
sys.path.append('radae')
from radae_base import CoreEncoder

# Load trained model
checkpoint = torch.load('model19_check3/checkpoints/checkpoint_epoch_100.pth',
                        map_location='cpu')

# Create encoder instance
encoder = CoreEncoder(
    latent_dim=80,
    EbNodB=100.0,
    state_dict=checkpoint['encoder_state_dict']
)
encoder.eval()

# Create dummy input (4 frames × 20 features = 80)
dummy_input = torch.randn(1, 4, 20)  # [batch, frames, features]

# Export to ONNX
torch.onnx.export(
    encoder,
    dummy_input,
    'radae_encoder.onnx',
    export_params=True,
    opset_version=13,
    do_constant_folding=True,
    input_names=['features'],
    output_names=['latent'],
    dynamic_axes={
        'features': {0: 'batch'},  # Variable batch size
        'latent': {0: 'batch'}
    }
)

print("Encoder exported to radae_encoder.onnx")

# Verify ONNX model
import onnx
model_onnx = onnx.load('radae_encoder.onnx')
onnx.checker.check_model(model_onnx)
print("ONNX model is valid")
```

**Export RADAE Decoder:**
```python
# export_decoder_onnx.py
import torch
import torch.onnx
from radae_base import CoreDecoder

checkpoint = torch.load('model19_check3/checkpoints/checkpoint_epoch_100.pth',
                        map_location='cpu')

decoder = CoreDecoder(
    latent_dim=80,
    state_dict=checkpoint['decoder_state_dict']
)
decoder.eval()

# Dummy input: latent vector (80 dims)
dummy_input = torch.randn(1, 80)

torch.onnx.export(
    decoder,
    dummy_input,
    'radae_decoder.onnx',
    export_params=True,
    opset_version=13,
    input_names=['latent'],
    output_names=['features'],  # 4 frames × 21 features
    dynamic_axes={
        'latent': {0: 'batch'},
        'features': {0: 'batch'}
    }
)

print("Decoder exported to radae_decoder.onnx")
```

**Simplify ONNX (Remove Redundant Ops):**
```bash
# Install ONNX simplifier
pip install onnx-simplifier

# Simplify encoder
python -m onnxsim radae_encoder.onnx radae_encoder_sim.onnx

# Simplify decoder
python -m onnxsim radae_decoder.onnx radae_decoder_sim.onnx

# Verify reduction in ops
python -c "
import onnx
orig = onnx.load('radae_encoder.onnx')
simp = onnx.load('radae_encoder_sim.onnx')
print(f'Original: {len(orig.graph.node)} ops')
print(f'Simplified: {len(simp.graph.node)} ops')
"
```

### 4.3 ONNX to TensorFlow Conversion

**Install ONNX-TensorFlow:**
```bash
pip install onnx-tf
```

**Convert Models:**
```python
# convert_onnx_to_tf.py
from onnx_tf.backend import prepare
import onnx

# Convert encoder
encoder_onnx = onnx.load('radae_encoder_sim.onnx')
encoder_tf = prepare(encoder_onnx)
encoder_tf.export_graph('radae_encoder_tf')

# Convert decoder
decoder_onnx = onnx.load('radae_decoder_sim.onnx')
decoder_tf = prepare(decoder_onnx)
decoder_tf.export_graph('radae_decoder_tf')

print("TensorFlow models saved to radae_encoder_tf/ and radae_decoder_tf/")
```

### 4.4 TensorFlow to TensorFlow Lite Conversion

**Representative Dataset for Quantization:**
```python
# create_calibration_dataset.py
import numpy as np
import torch
from radae.dataset import RADAEDataset

# Load training features for calibration
dataset = RADAEDataset(
    feature_file='~/Downloads/tts_speech_16k_speexdsp.f32',
    sequence_length=400
)

# Extract 100 samples for calibration
calibration_features = []
for i in range(100):
    features, _, _ = dataset[i]
    # Convert to [1, 4, 20] shape (encoder input)
    feat_4frames = features[:4, :20].numpy()
    calibration_features.append(feat_4frames)

# Save as .npy for TFLite converter
np.save('encoder_calibration.npy', np.array(calibration_features))

print(f"Calibration dataset: {len(calibration_features)} samples")
```

**Convert to TFLite with INT8 Quantization:**
```python
# convert_to_tflite_quantized.py
import tensorflow as tf
import numpy as np

# Representative dataset generator
def representative_dataset_encoder():
    calibration_data = np.load('encoder_calibration.npy').astype(np.float32)
    for sample in calibration_data:
        yield [np.expand_dims(sample, 0)]  # Add batch dimension

# Convert encoder
converter = tf.lite.TFLiteConverter.from_saved_model('radae_encoder_tf')

# Enable INT8 quantization
converter.optimizations = [tf.lite.Optimize.DEFAULT]
converter.representative_dataset = representative_dataset_encoder

# Force INT8 for all ops (including inputs/outputs)
converter.target_spec.supported_ops = [
    tf.lite.OpsSet.TFLITE_BUILTINS_INT8,
    tf.lite.OpsSet.TFLITE_BUILTINS  # Fallback for unsupported ops
]
converter.inference_input_type = tf.int8
converter.inference_output_type = tf.int8

# Convert
tflite_quant_encoder = converter.convert()

# Save
with open('radae_encoder_int8.tflite', 'wb') as f:
    f.write(tflite_quant_encoder)

print(f"Quantized encoder: {len(tflite_quant_encoder) / 1024:.1f} KB")

# Repeat for decoder
def representative_dataset_decoder():
    # Generate random latent vectors for calibration
    for _ in range(100):
        latent = np.random.randn(1, 80).astype(np.float32)
        yield [latent]

converter_dec = tf.lite.TFLiteConverter.from_saved_model('radae_decoder_tf')
converter_dec.optimizations = [tf.lite.Optimize.DEFAULT]
converter_dec.representative_dataset = representative_dataset_decoder
converter_dec.target_spec.supported_ops = [
    tf.lite.OpsSet.TFLITE_BUILTINS_INT8,
    tf.lite.OpsSet.TFLITE_BUILTINS
]
converter_dec.inference_input_type = tf.int8
converter_dec.inference_output_type = tf.int8

tflite_quant_decoder = converter_dec.convert()
with open('radae_decoder_int8.tflite', 'wb') as f:
    f.write(tflite_quant_decoder)

print(f"Quantized decoder: {len(tflite_quant_decoder) / 1024:.1f} KB")
```

### 4.5 Model Verification

**Test TFLite Model Accuracy:**
```python
# test_tflite_accuracy.py
import numpy as np
import tensorflow as tf
import torch
from radae_base import CoreEncoder

# Load original PyTorch model
checkpoint = torch.load('model19_check3/checkpoints/checkpoint_epoch_100.pth',
                        map_location='cpu')
encoder_torch = CoreEncoder(latent_dim=80, state_dict=checkpoint['encoder_state_dict'])
encoder_torch.eval()

# Load TFLite quantized model
interpreter = tf.lite.Interpreter(model_path='radae_encoder_int8.tflite')
interpreter.allocate_tensors()

input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()

print("Input details:", input_details)
print("Output details:", output_details)

# Test with sample input
test_input = np.random.randn(1, 4, 20).astype(np.float32)

# PyTorch inference
with torch.no_grad():
    output_torch = encoder_torch(torch.from_numpy(test_input)).numpy()

# TFLite inference (with quantization)
# Quantize input
input_scale = input_details[0]['quantization'][0]
input_zero_point = input_details[0]['quantization'][1]
test_input_quant = (test_input / input_scale + input_zero_point).astype(np.int8)

interpreter.set_tensor(input_details[0]['index'], test_input_quant)
interpreter.invoke()
output_tflite_quant = interpreter.get_tensor(output_details[0]['index'])

# Dequantize output
output_scale = output_details[0]['quantization'][0]
output_zero_point = output_details[0]['quantization'][1]
output_tflite = (output_tflite_quant.astype(np.float32) - output_zero_point) * output_scale

# Compare
mse = np.mean((output_torch - output_tflite)**2)
max_diff = np.max(np.abs(output_torch - output_tflite))

print(f"MSE between PyTorch and TFLite: {mse:.6f}")
print(f"Max absolute difference: {max_diff:.4f}")
print(f"Relative error: {mse / np.mean(output_torch**2) * 100:.2f}%")

# Expected: <5% relative error for INT8 quantization
```

### 4.6 FARGAN Vocoder Conversion

**Extract FARGAN Model from Opus:**
```python
# FARGAN is part of Opus, already in C
# For NPU acceleration, need to extract GRU layers

# Option 1: Keep FARGAN on CPU (lower priority, 175 MMAC LQ variant)
# Option 2: Convert FARGAN GRU to TFLite (advanced)

# For Phase 2, use CPU FARGAN
# Phase 3 can add NPU FARGAN if needed
```

---

## Phase 3: NPU Integration

**Duration:** 2-3 weeks
**Goal:** Integrate TFLite models with NPU delegate for hardware acceleration

### 5.1 NPU Delegate Configuration

**Install VeriSilicon NPU Delegate:**
```bash
# On i.MX 8M Plus
# Usually included in BSP, or install from NXP eIQ package

# Verify delegate library
ls -l /usr/lib/libvsi_npu.so
# Or
find /usr -name "*vsi*" -o -name "*npu*"

# Check NPU device node
ls -l /dev/galcore
sudo chmod 666 /dev/galcore  # If needed for testing
```

**TFLite Inference with NPU Delegate (Python):**
```python
# npu_inference.py
import numpy as np
import tflite_runtime.interpreter as tflite

# Load NPU delegate
try:
    npu_delegate = tflite.load_delegate('/usr/lib/libvsi_npu.so')
    print("NPU delegate loaded successfully")
except Exception as e:
    print(f"NPU delegate load failed: {e}")
    npu_delegate = None

# Create interpreter with NPU acceleration
interpreter = tflite.Interpreter(
    model_path='radae_encoder_int8.tflite',
    experimental_delegates=[npu_delegate] if npu_delegate else None
)

interpreter.allocate_tensors()

# Get input/output details
input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()

print("Model allocated to NPU")
print(f"Input: {input_details[0]['shape']}, dtype: {input_details[0]['dtype']}")
print(f"Output: {output_details[0]['shape']}, dtype: {output_details[0]['dtype']}")

# Test inference
def run_encoder_npu(features):
    """
    Run RADAE encoder on NPU
    Input: features [4, 20] float32
    Output: latent [80] float32
    """
    # Quantize input
    input_scale = input_details[0]['quantization'][0]
    input_zero_point = input_details[0]['quantization'][1]

    features_quant = (features / input_scale + input_zero_point).astype(np.int8)
    features_quant = np.expand_dims(features_quant, 0)  # Add batch dim

    # Run inference
    interpreter.set_tensor(input_details[0]['index'], features_quant)
    interpreter.invoke()

    # Get and dequantize output
    latent_quant = interpreter.get_tensor(output_details[0]['index'])
    output_scale = output_details[0]['quantization'][0]
    output_zero_point = output_details[0]['quantization'][1]
    latent = (latent_quant.astype(np.float32) - output_zero_point) * output_scale

    return latent[0]  # Remove batch dim

# Benchmark
import time
test_features = np.random.randn(4, 20).astype(np.float32)

# Warmup
for _ in range(10):
    _ = run_encoder_npu(test_features)

# Benchmark
num_runs = 100
start = time.time()
for _ in range(num_runs):
    latent = run_encoder_npu(test_features)
end = time.time()

avg_time_ms = (end - start) / num_runs * 1000
print(f"Average inference time: {avg_time_ms:.2f} ms")
print(f"Throughput: {1000 / avg_time_ms:.1f} inferences/second")

# Expected NPU performance:
# - Encoder: 3-5 ms/inference (vs 15-20 ms CPU)
# - Speedup: 4-6×
```

### 5.2 C/C++ NPU Integration

**TFLite C API Wrapper:**
```cpp
// npu_encoder.cpp
#include <tensorflow/lite/c/c_api.h>
#include <tensorflow/lite/delegates/external/external_delegate.h>
#include <cstring>
#include <cstdio>

class NPUEncoder {
private:
    TfLiteModel* model;
    TfLiteInterpreter* interpreter;
    TfLiteDelegate* npu_delegate;
    TfLiteTensor* input_tensor;
    const TfLiteTensor* output_tensor;

    float input_scale;
    int input_zero_point;
    float output_scale;
    int output_zero_point;

public:
    NPUEncoder(const char* model_path) {
        // Load model
        model = TfLiteModelCreateFromFile(model_path);
        if (!model) {
            fprintf(stderr, "Failed to load model\n");
            return;
        }

        // Create interpreter options
        TfLiteInterpreterOptions* options = TfLiteInterpreterOptionsCreate();
        TfLiteInterpreterOptionsSetNumThreads(options, 2);

        // Load NPU delegate
        TfLiteExternalDelegateOptions delegate_options =
            TfLiteExternalDelegateOptionsDefault("/usr/lib/libvsi_npu.so");
        npu_delegate = TfLiteExternalDelegateCreate(&delegate_options);

        if (npu_delegate) {
            TfLiteInterpreterOptionsAddDelegate(options, npu_delegate);
            printf("NPU delegate added\n");
        } else {
            printf("NPU delegate creation failed, using CPU\n");
        }

        // Create interpreter
        interpreter = TfLiteInterpreterCreate(model, options);
        TfLiteInterpreterOptionsDelete(options);

        // Allocate tensors
        if (TfLiteInterpreterAllocateTensors(interpreter) != kTfLiteOk) {
            fprintf(stderr, "Failed to allocate tensors\n");
            return;
        }

        // Get input/output tensors
        input_tensor = TfLiteInterpreterGetInputTensor(interpreter, 0);
        output_tensor = TfLiteInterpreterGetOutputTensor(interpreter, 0);

        // Get quantization parameters
        TfLiteQuantizationParams input_quant =
            TfLiteTensorQuantizationParams(input_tensor);
        input_scale = input_quant.scale;
        input_zero_point = input_quant.zero_point;

        TfLiteQuantizationParams output_quant =
            TfLiteTensorQuantizationParams(output_tensor);
        output_scale = output_quant.scale;
        output_zero_point = output_quant.zero_point;

        printf("Encoder initialized: input scale=%f, zp=%d\n",
               input_scale, input_zero_point);
    }

    ~NPUEncoder() {
        if (npu_delegate) TfLiteExternalDelegateDelete(npu_delegate);
        if (interpreter) TfLiteInterpreterDelete(interpreter);
        if (model) TfLiteModelDelete(model);
    }

    void encode(const float* features, float* latent) {
        // features: [4, 20] = 80 floats
        // latent: [80] floats

        // Quantize input
        int8_t* input_data = (int8_t*)TfLiteTensorData(input_tensor);
        for (int i = 0; i < 80; i++) {
            int32_t quant_val = (int32_t)(features[i] / input_scale) + input_zero_point;
            input_data[i] = (int8_t)std::min(127, std::max(-128, quant_val));
        }

        // Run inference
        if (TfLiteInterpreterInvoke(interpreter) != kTfLiteOk) {
            fprintf(stderr, "Inference failed\n");
            return;
        }

        // Dequantize output
        const int8_t* output_data = (const int8_t*)TfLiteTensorData(output_tensor);
        for (int i = 0; i < 80; i++) {
            latent[i] = (output_data[i] - output_zero_point) * output_scale;
        }
    }
};

// Test program
int main() {
    NPUEncoder encoder("radae_encoder_int8.tflite");

    float test_features[80];
    float latent[80];

    // Fill with random data
    for (int i = 0; i < 80; i++) {
        test_features[i] = ((float)rand() / RAND_MAX - 0.5f) * 2.0f;
    }

    // Warmup
    for (int i = 0; i < 10; i++) {
        encoder.encode(test_features, latent);
    }

    // Benchmark
    struct timespec start, end;
    int num_runs = 100;

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < num_runs; i++) {
        encoder.encode(test_features, latent);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    double elapsed = (end.tv_sec - start.tv_sec) +
                     (end.tv_nsec - start.tv_nsec) / 1e9;
    double avg_ms = elapsed / num_runs * 1000;

    printf("Average inference: %.2f ms\n", avg_ms);
    printf("Throughput: %.1f inferences/sec\n", 1000.0 / avg_ms);

    return 0;
}
```

**Compile C++ NPU Wrapper:**
```bash
# CMakeLists.txt for NPU encoder
cmake_minimum_required(VERSION 3.10)
project(radae_npu)

set(CMAKE_CXX_STANDARD 11)

# Find TensorFlow Lite
find_library(TFLITE_LIB tensorflow-lite
             PATHS /usr/lib /usr/local/lib)

# Add executable
add_executable(test_npu_encoder npu_encoder.cpp)
target_link_libraries(test_npu_encoder ${TFLITE_LIB} dl pthread)

# Compile and run
mkdir build-npu
cd build-npu
cmake ..
make
./test_npu_encoder
```

### 5.3 Streaming Implementation

**Stateful NPU Inference for Real-Time:**
```cpp
// radae_streaming.cpp
class RADAEStreamingProcessor {
private:
    NPUEncoder encoder;
    NPUDecoder decoder;

    // Buffers
    float feature_buffer[4][20];  // 4 frames × 20 features
    int buffer_fill;

public:
    RADAEStreamingProcessor() :
        encoder("radae_encoder_int8.tflite"),
        decoder("radae_decoder_int8.tflite"),
        buffer_fill(0) {}

    // Process single 10ms frame (20 features)
    bool process_frame(const float* features_10ms, float* output_features) {
        // Accumulate features
        memcpy(feature_buffer[buffer_fill], features_10ms, 20 * sizeof(float));
        buffer_fill++;

        if (buffer_fill < 4) {
            return false;  // Need 4 frames (40ms) before encoding
        }

        // Encode 4 frames → latent vector
        float latent[80];
        encoder.encode(&feature_buffer[0][0], latent);

        // TODO: Add OFDM modulation here
        // For now, directly decode

        // Decode latent → 4 frames × 21 features
        float decoded_features[4][21];
        decoder.decode(latent, &decoded_features[0][0]);

        // Output first frame, shift buffer
        memcpy(output_features, decoded_features[0], 21 * sizeof(float));

        // Shift buffer (overlap for continuous processing)
        for (int i = 0; i < 3; i++) {
            memcpy(feature_buffer[i], feature_buffer[i+1], 20 * sizeof(float));
        }
        buffer_fill = 3;

        return true;
    }
};
```

### 5.4 NPU Performance Optimization

**Multi-threaded Pipeline:**
```cpp
// Pipeline: Feature extraction (CPU) → Encoder (NPU) → OFDM (CPU) → ...

#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

class PipelineProcessor {
private:
    std::queue<float*> feature_queue;
    std::queue<float*> latent_queue;
    std::mutex mtx;
    std::condition_variable cv;
    bool running;

    NPUEncoder encoder;

    void npu_worker() {
        while (running) {
            float* features = nullptr;

            {
                std::unique_lock<std::mutex> lock(mtx);
                cv.wait(lock, [this] { return !feature_queue.empty() || !running; });

                if (!running && feature_queue.empty()) break;

                features = feature_queue.front();
                feature_queue.pop();
            }

            if (features) {
                float latent[80];
                encoder.encode(features, latent);

                {
                    std::lock_guard<std::mutex> lock(mtx);
                    latent_queue.push(latent);
                }

                delete[] features;
            }
        }
    }

public:
    PipelineProcessor() : running(true), encoder("radae_encoder_int8.tflite") {
        // Start NPU worker thread
        std::thread worker(&PipelineProcessor::npu_worker, this);
        worker.detach();
    }

    void submit_features(const float* features) {
        float* features_copy = new float[80];
        memcpy(features_copy, features, 80 * sizeof(float));

        {
            std::lock_guard<std::mutex> lock(mtx);
            feature_queue.push(features_copy);
        }
        cv.notify_one();
    }
};
```

---

## Phase 4: System Integration

**Duration:** 1-2 weeks
**Goal:** Complete RADAE system with NPU acceleration

### 6.1 Full System Architecture

```
┌─────────────────────────────────────────────────────────┐
│                   i.MX 8M Plus System                    │
├─────────────────────────────────────────────────────────┤
│                                                           │
│  ┌─────────────┐  ┌──────────────┐  ┌────────────────┐ │
│  │   Audio     │  │   Feature    │  │   RADAE        │ │
│  │  Capture    │→ │  Extraction  │→ │   Encoder      │ │
│  │  (ALSA)     │  │  (CPU/NEON)  │  │   (NPU)        │ │
│  └─────────────┘  └──────────────┘  └────────────────┘ │
│                                             ↓            │
│                                      ┌────────────────┐ │
│                                      │   OFDM Tx      │ │
│                                      │   (CPU/NEON)   │ │
│                                      └────────────────┘ │
│                                             ↓            │
│                                      ┌────────────────┐ │
│                                      │   RF/SDR       │ │
│                                      │   Interface    │ │
│                                      └────────────────┘ │
│                                             ↓            │
│                                      ┌────────────────┐ │
│                                      │   OFDM Rx      │ │
│                                      │   (CPU/NEON)   │ │
│                                      └────────────────┘ │
│                                             ↓            │
│  ┌─────────────┐  ┌──────────────┐  ┌────────────────┐ │
│  │   Audio     │← │   FARGAN     │← │   RADAE        │ │
│  │  Playback   │  │   Vocoder    │  │   Decoder      │ │
│  │  (ALSA)     │  │  (CPU/NEON)  │  │   (NPU)        │ │
│  └─────────────┘  └──────────────┘  └────────────────┘ │
│                                                           │
└─────────────────────────────────────────────────────────┘
```

### 6.2 Audio Interface Integration

**ALSA Audio Capture:**
```c
// audio_capture.c
#include <alsa/asoundlib.h>

#define SAMPLE_RATE 16000
#define CHANNELS 1
#define PERIOD_SIZE 160  // 10ms @ 16kHz

typedef struct {
    snd_pcm_t *capture_handle;
    snd_pcm_t *playback_handle;
} AudioInterface;

int audio_init(AudioInterface *audio) {
    int err;
    snd_pcm_hw_params_t *hw_params;

    // Open capture device
    if ((err = snd_pcm_open(&audio->capture_handle, "default",
                            SND_PCM_STREAM_CAPTURE, 0)) < 0) {
        fprintf(stderr, "Cannot open capture device: %s\n", snd_strerror(err));
        return -1;
    }

    // Configure capture
    snd_pcm_hw_params_alloca(&hw_params);
    snd_pcm_hw_params_any(audio->capture_handle, hw_params);
    snd_pcm_hw_params_set_access(audio->capture_handle, hw_params,
                                  SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(audio->capture_handle, hw_params,
                                  SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(audio->capture_handle, hw_params, CHANNELS);
    snd_pcm_hw_params_set_rate_near(audio->capture_handle, hw_params,
                                     &SAMPLE_RATE, 0);
    snd_pcm_hw_params_set_period_size(audio->capture_handle, hw_params,
                                       PERIOD_SIZE, 0);

    if ((err = snd_pcm_hw_params(audio->capture_handle, hw_params)) < 0) {
        fprintf(stderr, "Cannot set parameters: %s\n", snd_strerror(err));
        return -1;
    }

    snd_pcm_prepare(audio->capture_handle);

    // Similar setup for playback...

    return 0;
}

int audio_read_frame(AudioInterface *audio, int16_t *buffer) {
    int err = snd_pcm_readi(audio->capture_handle, buffer, PERIOD_SIZE);
    if (err != PERIOD_SIZE) {
        if (err == -EPIPE) {
            snd_pcm_prepare(audio->capture_handle);  // Recover from overrun
        }
        return -1;
    }
    return 0;
}
```

### 6.3 Complete TX/RX Pipeline

**Main Application:**
```c
// radae_main.c
#include "audio_capture.h"
#include "fargan_features.h"
#include "npu_encoder.h"
#include "ofdm_modem.h"

int main(int argc, char **argv) {
    AudioInterface audio;
    FARGANExtractor fargan;
    NPUEncoder encoder;
    OFDMModulator ofdm_tx;

    // Initialize components
    audio_init(&audio);
    fargan_init(&fargan, "fargan_model.bin");
    encoder_init(&encoder, "radae_encoder_int8.tflite");
    ofdm_init(&ofdm_tx, 8000, 30, 50);  // Fs=8kHz, Nc=30, Rs=50Hz

    int16_t audio_samples[PERIOD_SIZE];
    float features[20];
    float feature_buffer[4][20];
    int frame_count = 0;

    printf("RADAE transmitter started\n");

    while (1) {
        // Capture audio (10ms frame)
        if (audio_read_frame(&audio, audio_samples) < 0) {
            continue;
        }

        // Extract FARGAN features (CPU)
        fargan_extract_features(&fargan, audio_samples, PERIOD_SIZE, features);

        // Buffer 4 frames (40ms)
        memcpy(feature_buffer[frame_count % 4], features, 20 * sizeof(float));
        frame_count++;

        if (frame_count % 4 == 0) {
            // Encode with NPU (4 frames → 80 latent)
            float latent[80];
            encoder_run(&encoder, &feature_buffer[0][0], latent);

            // OFDM modulate (CPU)
            float complex iq_samples[960];  // 120ms @ 8kHz
            ofdm_modulate(&ofdm_tx, latent, iq_samples);

            // Send to SDR/RF interface
            // sdr_transmit(iq_samples, 960);
        }
    }

    return 0;
}
```

### 6.4 Build System Integration

**Complete CMake Build:**
```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(radae_imx8mp C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 14)

# Optimization flags for i.MX 8M Plus
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -march=armv8-a+simd -mtune=cortex-a53")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=armv8-a+simd -mtune=cortex-a53")

# Find dependencies
find_package(PkgConfig REQUIRED)
pkg_check_modules(ALSA REQUIRED alsa)

find_library(TFLITE_LIB tensorflow-lite REQUIRED)

# FARGAN vocoder (from Opus)
add_subdirectory(opus)

# NPU encoder/decoder wrappers
add_library(radae_npu STATIC
    src/npu_encoder.cpp
    src/npu_decoder.cpp
)
target_link_libraries(radae_npu ${TFLITE_LIB} dl pthread)

# OFDM modem (CPU with NEON)
add_library(ofdm_neon STATIC
    src/ofdm_neon.c
    src/fft_neon.c
)

# Main application
add_executable(radae_tx
    src/radae_main.c
    src/audio_capture.c
)

target_link_libraries(radae_tx
    radae_npu
    ofdm_neon
    opus_fargan
    ${ALSA_LIBRARIES}
    m
    pthread
)

# Install
install(TARGETS radae_tx DESTINATION bin)
install(FILES
    models/radae_encoder_int8.tflite
    models/radae_decoder_int8.tflite
    DESTINATION share/radae)
```

---

## 7. Performance Optimization

### 7.1 NPU Utilization Profiling

**Monitor NPU Usage:**
```bash
# Check NPU frequency
cat /sys/kernel/debug/gc/clk

# Monitor NPU load (if available)
cat /sys/class/misc/galcore/device/utilization

# Profile with i.MX tools
imx-gputop  # If available in BSP
```

**TensorFlow Lite Profiling:**
```python
# profile_npu.py
import tflite_runtime.interpreter as tflite
import numpy as np

# Enable profiling
interpreter = tflite.Interpreter(
    model_path='radae_encoder_int8.tflite',
    experimental_delegates=[tflite.load_delegate('/usr/lib/libvsi_npu.so')]
)
interpreter.allocate_tensors()

# Run with profiling
profiling_data = interpreter.get_profiling_info()
for item in profiling_data:
    print(f"{item['node_type']}: {item['elapsed_time_us']} us")
```

### 7.2 Memory Optimization

**Reduce Memory Footprint:**
```c
// Use memory mapping for models (avoid loading into RAM)
#include <sys/mman.h>
#include <fcntl.h>

void* load_model_mmap(const char* path, size_t *size) {
    int fd = open(path, O_RDONLY);
    *size = lseek(fd, 0, SEEK_END);

    void* data = mmap(NULL, *size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    return data;
}

// Use with TFLite
size_t model_size;
void* model_data = load_model_mmap("radae_encoder_int8.tflite", &model_size);
TfLiteModel* model = TfLiteModelCreate(model_data, model_size);
```

### 7.3 Power Management

**Dynamic CPU Frequency Scaling:**
```bash
# Set CPU governor to performance during RADAE operation
for cpu in /sys/devices/system/cpu/cpu[0-3]; do
    echo performance > $cpu/cpufreq/scaling_governor
done

# Or use ondemand for power saving
echo ondemand > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
```

**Application-level Power Control:**
```c
// radae_power.c
#include <sched.h>

void optimize_scheduling() {
    // Set real-time priority for audio thread
    struct sched_param param;
    param.sched_priority = 80;
    sched_setscheduler(0, SCHED_FIFO, &param);

    // Pin NPU worker to specific cores
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(2, &cpuset);  // Use CPU 2 for NPU worker
    CPU_SET(3, &cpuset);  // Use CPU 3 for NPU worker
    sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);
}
```

---

## 8. Testing and Validation

### 8.1 Unit Tests

**NPU Encoder/Decoder Tests:**
```cpp
// test_npu_accuracy.cpp
#include <gtest/gtest.h>
#include "npu_encoder.h"
#include "npu_decoder.h"

TEST(NPUTest, EncoderAccuracy) {
    NPUEncoder encoder("radae_encoder_int8.tflite");

    // Load reference data from PyTorch
    float features[80], expected_latent[80], actual_latent[80];
    load_test_data("test_features.bin", features);
    load_test_data("test_latent.bin", expected_latent);

    encoder.encode(features, actual_latent);

    // Check MSE < 0.05
    float mse = 0;
    for (int i = 0; i < 80; i++) {
        float diff = actual_latent[i] - expected_latent[i];
        mse += diff * diff;
    }
    mse /= 80;

    EXPECT_LT(mse, 0.05);
}

TEST(NPUTest, Throughput) {
    NPUEncoder encoder("radae_encoder_int8.tflite");

    float features[80], latent[80];
    for (int i = 0; i < 80; i++) features[i] = ((float)rand() / RAND_MAX - 0.5) * 2;

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100; i++) {
        encoder.encode(features, latent);
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    float avg_us = duration.count() / 100.0;

    EXPECT_LT(avg_us, 5000);  // <5ms per inference

    std::cout << "Average NPU inference: " << avg_us / 1000 << " ms\n";
}
```

### 8.2 End-to-End Testing

**Loopback Test:**
```bash
# Test TX → RX pipeline with loopback
./radae_tx --input test.wav --output tx.iq
./radae_rx --input tx.iq --output rx.wav

# Compare audio quality
python3 loss.py --input1 test_features.f32 --input2 rx_features.f32
# Expected loss: <0.15 (similar to Python implementation)
```

### 8.3 Performance Benchmarking

**Full System Benchmark:**
```c
// benchmark.c
void benchmark_full_pipeline() {
    // Simulate 10 seconds of audio processing
    int num_frames = 1000;  // 10ms frames

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < num_frames; i++) {
        // Feature extraction
        fargan_extract_features(...);

        // NPU encoding (every 4 frames)
        if (i % 4 == 0) {
            encoder_run(...);
        }

        // OFDM
        ofdm_modulate(...);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    double elapsed = (end.tv_sec - start.tv_sec) +
                     (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("Processed 10s audio in %.2f seconds\n", elapsed);
    printf("Real-time factor: %.1f×\n", 10.0 / elapsed);
}
```

---

## 9. Deployment

### 9.1 System Image Creation

**Yocto Recipe for RADAE:**
```bitbake
# radae_1.0.bb
SUMMARY = "RADAE Neural Codec for i.MX 8M Plus"
LICENSE = "BSD-2-Clause"

SRC_URI = "git://github.com/drowe67/radae.git;protocol=https;branch=main"
SRCREV = "${AUTOREV}"

DEPENDS = "tensorflow-lite opus alsa-lib"

inherit cmake

EXTRA_OECMAKE = " \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_NEON=ON \
    -DENABLE_NPU=ON \
"

do_install_append() {
    install -d ${D}${datadir}/radae
    install -m 0644 ${B}/models/*.tflite ${D}${datadir}/radae/
}

FILES_${PN} += "${datadir}/radae/*.tflite"
```

### 9.2 Application Packaging

**Debian Package:**
```bash
# Create .deb package
mkdir -p radae-imx8mp_1.0/DEBIAN
mkdir -p radae-imx8mp_1.0/usr/bin
mkdir -p radae-imx8mp_1.0/usr/share/radae

# Control file
cat > radae-imx8mp_1.0/DEBIAN/control << EOF
Package: radae-imx8mp
Version: 1.0
Architecture: arm64
Maintainer: Your Name <your.email@example.com>
Description: RADAE Neural Codec optimized for i.MX 8M Plus NPU
 Real-time speech codec for HF radio using neural autoencoder
 with NPU acceleration.
Depends: libasound2, tensorflow-lite-runtime
EOF

# Copy binaries
cp build/radae_tx radae-imx8mp_1.0/usr/bin/
cp models/*.tflite radae-imx8mp_1.0/usr/share/radae/

# Build package
dpkg-deb --build radae-imx8mp_1.0
```

### 9.3 Systemd Service

**Auto-start RADAE service:**
```ini
# /etc/systemd/system/radae.service
[Unit]
Description=RADAE HF Voice Codec
After=network.target sound.target

[Service]
Type=simple
User=root
ExecStartPre=/usr/bin/chrt -f -p 80 $$
ExecStart=/usr/bin/radae_tx --config /etc/radae/config.conf
Restart=on-failure
RestartSec=5s

# Resource limits
LimitRTPRIO=95
LimitMEMLOCK=infinity
Nice=-10

# Power management
CPUAffinity=0 1 2 3

[Install]
WantedBy=multi-user.target
```

---

## 10. Expected Results Summary

### Performance Metrics (i.MX 8M Plus)

| Metric | CPU-Only | NPU-Accelerated | Improvement |
|--------|----------|-----------------|-------------|
| **Encoder (40ms)** | 15-20 ms | 3-5 ms | **4-5×** |
| **Decoder (40ms)** | 15-20 ms | 3-5 ms | **4-5×** |
| **FARGAN (10ms)** | 20-25 ms | 20-25 ms (CPU) | 1× |
| **Total (10ms frame)** | 12-15 ms | 6-8 ms | **2×** |
| **Real-time Factor** | 1.5-2× | **8-10×** | **5×** |
| **Power (ML only)** | 5-8W | 1.5-2.5W | **-65%** |
| **System Power** | 8-10W | 3-5W | **-55%** |

### Resource Utilization

| Resource | Utilization | Notes |
|----------|-------------|-------|
| **CPU (4 cores)** | 25-40% | OFDM, FARGAN, control |
| **NPU** | 40-60% | Encoder/decoder inference |
| **RAM** | 150-250 MB | Models + buffers |
| **Storage** | 15-20 MB | TFLite models + binary |

### Quality Metrics

| Metric | Target | Expected |
|--------|--------|----------|
| **Feature Loss (vs PyTorch)** | <0.15 | 0.10-0.14 |
| **Quantization Error** | <5% | 2-4% |
| **End-to-End Latency** | <200ms | 180-185ms |
| **Audio Quality (MOS)** | >3.5 | 3.6-3.9 |

---

## 11. Troubleshooting Guide

### Common Issues

**1. NPU Delegate Not Loading:**
```bash
# Check NPU driver
lsmod | grep galcore
# If not loaded:
modprobe galcore

# Check permissions
ls -l /dev/galcore
chmod 666 /dev/galcore  # For testing only
```

**2. TFLite Model Crashes:**
```python
# Verify model with interpreter
import tflite_runtime.interpreter as tflite

interpreter = tflite.Interpreter(model_path='radae_encoder_int8.tflite')
interpreter.allocate_tensors()

# Check tensor details
input_details = interpreter.get_input_details()
print("Input shape:", input_details[0]['shape'])
print("Input dtype:", input_details[0]['dtype'])
```

**3. Poor NPU Performance:**
```bash
# Check NPU frequency scaling
cat /sys/class/misc/galcore/device/clk

# Lock to max frequency
echo performance > /sys/class/misc/galcore/device/governor
```

**4. Audio Dropouts:**
```c
// Increase ALSA buffer size
snd_pcm_hw_params_set_buffer_size(handle, params, 8192);  // Larger buffer
snd_pcm_hw_params_set_periods(handle, params, 4, 0);      // More periods
```

---

## 12. Next Steps and Future Optimizations

### Phase 5: Advanced Optimizations (Optional)

1. **FARGAN NPU Acceleration**
   - Convert FARGAN GRU layers to TFLite
   - Expected: 5-8× speedup on NPU
   - Power savings: Additional 30-40%

2. **Multi-Model Pipeline**
   - Run encoder and decoder simultaneously on NPU
   - Full-duplex operation with <3W power

3. **Fixed-Point OFDM**
   - Convert OFDM DSP to fixed-point NEON
   - 2-3× speedup on OFDM processing

4. **Custom NPU Kernels**
   - Write optimized kernels for specific layers
   - Potential 10-20% additional speedup

### Long-Term Roadmap

- **RADEv2 Integration:** ML-based sync/equalization on NPU
- **Adaptive Bitrate:** Switch models based on channel conditions
- **Multi-Channel:** Support multiple simultaneous channels
- **GPU Acceleration:** Offload OFDM FFT to GPU

---

## Appendix A: File Structure

```
radae-imx8mp/
├── models/
│   ├── radae_encoder_int8.tflite
│   ├── radae_decoder_int8.tflite
│   └── fargan_model.bin
├── src/
│   ├── npu_encoder.cpp
│   ├── npu_decoder.cpp
│   ├── audio_capture.c
│   ├── ofdm_neon.c
│   ├── fft_neon.c
│   └── radae_main.c
├── include/
│   ├── npu_encoder.h
│   ├── audio_capture.h
│   └── ofdm_neon.h
├── scripts/
│   ├── export_models.py
│   ├── convert_to_tflite.py
│   └── benchmark.sh
├── tests/
│   ├── test_npu_accuracy.cpp
│   └── test_full_pipeline.c
├── CMakeLists.txt
└── README.md
```

---

## Appendix B: Quick Reference Commands

**Build Commands:**
```bash
# Cross-compile
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../toolchain-imx8mp.cmake
make -j4

# Native build on i.MX 8M Plus
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4
```

**Model Conversion:**
```bash
# PyTorch → ONNX
python scripts/export_encoder_onnx.py

# ONNX → TFLite INT8
python scripts/convert_to_tflite.py --quantize
```

**Testing:**
```bash
# Unit tests
./build/test_npu_accuracy

# Benchmark
./build/radae_tx --benchmark

# Full pipeline
./build/radae_tx --input test.wav --output out.wav
```

**Deployment:**
```bash
# Install
sudo make install

# Run as service
sudo systemctl enable radae
sudo systemctl start radae
```

---

**Document Version:** 1.0
**Last Updated:** 2025-11-16
**Author:** RADAE Implementation Team
**Platform:** NXP i.MX 8M Plus with NPU
