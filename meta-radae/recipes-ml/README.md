# RADAE Machine Learning Recipes

This directory contains Yocto/BitBake recipes for building and deploying RADAE neural network models on embedded systems, specifically targeting the i.MX 8M Plus NPU.

## Overview

The recipes provide a complete pipeline for:
1. Converting PyTorch models to ONNX format
2. Quantizing models to INT8 for NPU acceleration
3. Converting to TensorFlow Lite format
4. Packaging models for deployment
5. Verifying model accuracy after conversion

## Recipes

### 1. radae-model-converter

**Recipe:** `radae-model-converter/radae-model-converter_1.0.bb`

Provides model conversion tools and scripts.

**Installed Commands:**
- `radae-export-models` - Export PyTorch models to ONNX
- `radae-convert-tflite` - Convert ONNX to TFLite with quantization
- `radae-verify-accuracy` - Verify converted model accuracy
- `radae-full-conversion` - Complete conversion pipeline

### 2. radae-tflite-models

**Recipe:** `radae-tflite-models/radae-tflite-models_1.0.bb`

Pre-builds and packages INT8 quantized TFLite models.

**Models Installed:** `/usr/share/radae/tflite/`
- `encoder_int8.tflite` - INT8 quantized encoder
- `decoder_int8.tflite` - INT8 quantized decoder

### 3. python3-radae

**Recipe:** `python3-radae/python3-radae_1.0.bb`

Packages RADAE as a Python module.

## Building

```bash
bitbake radae-model-converter
bitbake radae-tflite-models
bitbake python3-radae
```

See README.md for detailed documentation.
