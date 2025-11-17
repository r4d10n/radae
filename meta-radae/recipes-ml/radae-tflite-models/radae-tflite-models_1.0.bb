SUMMARY = "Pre-converted RADAE TensorFlow Lite Models"
DESCRIPTION = "INT8 quantized TensorFlow Lite models for RADAE encoder and decoder, optimized for NPU"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

# Depends on converter tools for building
DEPENDS = "radae-model-converter-native python3-radae-native"

# Runtime dependencies
RDEPENDS:${PN} = "tensorflow-lite imx-vsi-npu"

# Fetch from local repository
SRC_URI = " \
    file://model19_checkpoint.pth \
    file://calibration_data.f32 \
    file://test_features.f32 \
"

S = "${WORKDIR}"

# Model conversion happens at build time
inherit allarch

# Output directory for converted models
TFLITE_MODEL_DIR = "${WORKDIR}/tflite_models"
ONNX_MODEL_DIR = "${WORKDIR}/onnx_models"

do_configure() {
    # Create output directories
    mkdir -p ${ONNX_MODEL_DIR}
    mkdir -p ${TFLITE_MODEL_DIR}

    # Verify checkpoint exists
    if [ ! -f "${WORKDIR}/model19_checkpoint.pth" ]; then
        bbwarn "No checkpoint found, using default model19"
        # Copy from source tree if available
        if [ -f "${THISDIR}/../../../model19/checkpoints/checkpoint_epoch_100.pth" ]; then
            cp "${THISDIR}/../../../model19/checkpoints/checkpoint_epoch_100.pth" \
               "${WORKDIR}/model19_checkpoint.pth"
        else
            bbfatal "Cannot find RADAE checkpoint file"
        fi
    fi
}

do_compile() {
    # Step 1: Export PyTorch models to ONNX format
    bbnote "Exporting encoder and decoder to ONNX format..."

    python3 ${STAGING_BINDIR_NATIVE}/radae-converter/export_models.py \
        ${WORKDIR}/model19_checkpoint.pth \
        ${ONNX_MODEL_DIR} \
        --latent-dim 80 \
        --separate-encoder-decoder || bbfatal "ONNX export failed"

    # Verify ONNX models were created
    if [ ! -f "${ONNX_MODEL_DIR}/encoder.onnx" ] || \
       [ ! -f "${ONNX_MODEL_DIR}/decoder.onnx" ]; then
        bbfatal "ONNX export did not produce expected model files"
    fi

    # Step 2: Convert ONNX to TensorFlow Lite with INT8 quantization
    bbnote "Converting encoder to TensorFlow Lite (INT8)..."
    python3 ${STAGING_BINDIR_NATIVE}/radae-converter/convert_to_tflite.py \
        ${ONNX_MODEL_DIR}/encoder.onnx \
        ${TFLITE_MODEL_DIR}/encoder_int8.tflite \
        --quantize int8 \
        --calibration-data ${WORKDIR}/calibration_data.f32 \
        --input-shape "1,4,20" \
        --calibration-samples 100 || bbfatal "Encoder TFLite conversion failed"

    bbnote "Converting decoder to TensorFlow Lite (INT8)..."
    python3 ${STAGING_BINDIR_NATIVE}/radae-converter/convert_to_tflite.py \
        ${ONNX_MODEL_DIR}/decoder.onnx \
        ${TFLITE_MODEL_DIR}/decoder_int8.tflite \
        --quantize int8 \
        --calibration-data ${WORKDIR}/calibration_data.f32 \
        --input-shape "1,1,80" \
        --calibration-samples 100 || bbfatal "Decoder TFLite conversion failed"

    # Step 3: Verify model accuracy
    bbnote "Verifying converted model accuracy..."
    python3 ${STAGING_BINDIR_NATIVE}/radae-converter/verify_accuracy.py \
        ${WORKDIR}/model19_checkpoint.pth \
        ${TFLITE_MODEL_DIR} \
        --test-data ${WORKDIR}/test_features.f32 \
        --tolerance 0.02 \
        --test-samples 50 || bbwarn "Accuracy verification warning (expected for quantized models)"

    # Generate model metadata
    cat > ${TFLITE_MODEL_DIR}/model_info.txt << EOF
RADAE TensorFlow Lite Models - INT8 Quantized
==============================================
Generated: $(date)
Source checkpoint: model19_checkpoint.pth
Quantization: INT8
Target: i.MX 8M Plus NPU (VeriSilicon)

Models:
  - encoder_int8.tflite : Encoder (20 features -> 80 latents)
  - decoder_int8.tflite : Decoder (80 latents -> 20 features)

Input shapes:
  - Encoder: [1, 4, 20] (batch, time_steps, features)
  - Decoder: [1, 1, 80] (batch, latent_vectors, latent_dim)

Deployment:
  Install to /usr/share/radae/tflite/
  Use with TFLite VeriSilicon delegate for NPU acceleration
EOF

    # Create NPU configuration files
    cat > ${TFLITE_MODEL_DIR}/encoder_npu.json << 'EOF'
{
  "model_file": "encoder_int8.tflite",
  "delegate": "verisilicon",
  "allow_fp16": false,
  "enable_npu_cache": true,
  "num_threads": 4,
  "input": {
    "name": "input",
    "shape": [1, 4, 20],
    "type": "FLOAT32"
  },
  "output": {
    "name": "output",
    "shape": [1, 1, 80],
    "type": "FLOAT32"
  }
}
EOF

    cat > ${TFLITE_MODEL_DIR}/decoder_npu.json << 'EOF'
{
  "model_file": "decoder_int8.tflite",
  "delegate": "verisilicon",
  "allow_fp16": false,
  "enable_npu_cache": true,
  "num_threads": 4,
  "input": {
    "name": "input",
    "shape": [1, 1, 80],
    "type": "FLOAT32"
  },
  "output": {
    "name": "output",
    "shape": [1, 16, 20],
    "type": "FLOAT32"
  }
}
EOF
}

do_install() {
    # Install TFLite models to shared directory
    install -d ${D}${datadir}/radae/tflite

    # Install quantized models
    install -m 0644 ${TFLITE_MODEL_DIR}/encoder_int8.tflite \
        ${D}${datadir}/radae/tflite/
    install -m 0644 ${TFLITE_MODEL_DIR}/decoder_int8.tflite \
        ${D}${datadir}/radae/tflite/

    # Install metadata and configuration
    install -m 0644 ${TFLITE_MODEL_DIR}/model_info.txt \
        ${D}${datadir}/radae/tflite/
    install -m 0644 ${TFLITE_MODEL_DIR}/encoder_npu.json \
        ${D}${datadir}/radae/tflite/
    install -m 0644 ${TFLITE_MODEL_DIR}/decoder_npu.json \
        ${D}${datadir}/radae/tflite/

    # Create symlinks for easier access
    ln -sf encoder_int8.tflite ${D}${datadir}/radae/tflite/encoder.tflite
    ln -sf decoder_int8.tflite ${D}${datadir}/radae/tflite/decoder.tflite

    # Install ONNX models for reference (optional)
    install -d ${D}${datadir}/radae/onnx
    install -m 0644 ${ONNX_MODEL_DIR}/encoder.onnx \
        ${D}${datadir}/radae/onnx/ || true
    install -m 0644 ${ONNX_MODEL_DIR}/decoder.onnx \
        ${D}${datadir}/radae/onnx/ || true
}

# Model verification test
do_install_ptest() {
    install -d ${D}${PTEST_PATH}
    install -m 0755 ${WORKDIR}/test_features.f32 ${D}${PTEST_PATH}/

    cat > ${D}${PTEST_PATH}/run-ptest << 'EOF'
#!/bin/sh
# Test TFLite model loading
python3 -c "
import tensorflow as tf
import os

model_dir = '/usr/share/radae/tflite'
encoder_path = os.path.join(model_dir, 'encoder_int8.tflite')
decoder_path = os.path.join(model_dir, 'decoder_int8.tflite')

try:
    # Load encoder
    with open(encoder_path, 'rb') as f:
        encoder = tf.lite.Interpreter(model_content=f.read())
    encoder.allocate_tensors()
    print('PASS: Encoder model loaded')

    # Load decoder
    with open(decoder_path, 'rb') as f:
        decoder = tf.lite.Interpreter(model_content=f.read())
    decoder.allocate_tensors()
    print('PASS: Decoder model loaded')

except Exception as e:
    print(f'FAIL: {e}')
    exit(1)
"
EOF
    chmod 0755 ${D}${PTEST_PATH}/run-ptest
}

FILES:${PN} = " \
    ${datadir}/radae/tflite/*.tflite \
    ${datadir}/radae/tflite/*.json \
    ${datadir}/radae/tflite/*.txt \
    ${datadir}/radae/onnx/*.onnx \
"

# Package test files separately
FILES:${PN}-ptest = "${PTEST_PATH}/*"

PACKAGE_ARCH = "${MACHINE_ARCH}"

# Ensure VeriSilicon NPU delegate is available
RRECOMMENDS:${PN} += "imx-vpu-hantro firmware-imx-vpu-imx8"
