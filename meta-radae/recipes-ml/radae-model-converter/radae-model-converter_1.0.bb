SUMMARY = "RADAE Model Conversion Tools"
DESCRIPTION = "Tools for converting RADAE PyTorch models to ONNX and TensorFlow Lite formats"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

# Dependencies for model conversion
DEPENDS = "python3-native python3-pip-native"

# Runtime dependencies
RDEPENDS:${PN} = " \
    python3-torch \
    python3-numpy \
    python3-onnx \
    python3-onnxruntime \
    python3-tensorflow-lite \
    python3-tf2onnx \
"

SRC_URI = " \
    file://export_models.py \
    file://convert_to_tflite.py \
    file://verify_accuracy.py \
    file://calibration_dataset.py \
    file://quantize_model.py \
"

S = "${WORKDIR}"

inherit setuptools3

# Native build for model conversion on build host
BBCLASSEXTEND = "native nativesdk"

do_configure() {
    # Verify Python dependencies
    python3 - << 'EOF'
import sys
try:
    import torch
    import onnx
    import numpy
    print("Python dependencies available")
except ImportError as e:
    print(f"Warning: Missing dependency: {e}", file=sys.stderr)
    sys.exit(0)
EOF
}

do_compile() {
    # Pre-compile conversion scripts
    ${PYTHON} -m py_compile ${WORKDIR}/export_models.py
    ${PYTHON} -m py_compile ${WORKDIR}/convert_to_tflite.py
    ${PYTHON} -m py_compile ${WORKDIR}/verify_accuracy.py
    ${PYTHON} -m py_compile ${WORKDIR}/calibration_dataset.py
    ${PYTHON} -m py_compile ${WORKDIR}/quantize_model.py
}

do_install() {
    install -d ${D}${bindir}/radae-converter
    install -d ${D}${datadir}/radae/converter
    install -d ${D}${datadir}/radae/converter/scripts

    # Install conversion scripts
    install -m 0755 ${WORKDIR}/export_models.py ${D}${bindir}/radae-converter/
    install -m 0755 ${WORKDIR}/convert_to_tflite.py ${D}${bindir}/radae-converter/
    install -m 0755 ${WORKDIR}/verify_accuracy.py ${D}${bindir}/radae-converter/
    install -m 0644 ${WORKDIR}/calibration_dataset.py ${D}${datadir}/radae/converter/
    install -m 0644 ${WORKDIR}/quantize_model.py ${D}${datadir}/radae/converter/

    # Create helper wrapper scripts
    cat > ${D}${bindir}/radae-export-models << 'EOF'
#!/bin/sh
# Export PyTorch models to ONNX format
exec python3 ${bindir}/radae-converter/export_models.py "$@"
EOF
    chmod 0755 ${D}${bindir}/radae-export-models

    cat > ${D}${bindir}/radae-convert-tflite << 'EOF'
#!/bin/sh
# Convert ONNX models to TensorFlow Lite with quantization
exec python3 ${bindir}/radae-converter/convert_to_tflite.py "$@"
EOF
    chmod 0755 ${D}${bindir}/radae-convert-tflite

    cat > ${D}${bindir}/radae-verify-accuracy << 'EOF'
#!/bin/sh
# Verify converted model accuracy
exec python3 ${bindir}/radae-converter/verify_accuracy.py "$@"
EOF
    chmod 0755 ${D}${bindir}/radae-verify-accuracy

    # Create conversion pipeline script
    cat > ${D}${bindir}/radae-full-conversion << 'EOF'
#!/bin/bash
# Full conversion pipeline: PyTorch -> ONNX -> TFLite (INT8)
set -e

if [ $# -lt 2 ]; then
    echo "Usage: $0 <checkpoint.pth> <output_dir>"
    exit 1
fi

CHECKPOINT="$1"
OUTPUT_DIR="$2"

echo "Starting RADAE model conversion pipeline..."
echo "Checkpoint: $CHECKPOINT"
echo "Output directory: $OUTPUT_DIR"

# Step 1: Export to ONNX
echo ""
echo "Step 1/3: Exporting PyTorch models to ONNX..."
radae-export-models "$CHECKPOINT" "$OUTPUT_DIR/onnx" --latent-dim 80

# Step 2: Convert to TFLite with INT8 quantization
echo ""
echo "Step 2/3: Converting ONNX to TensorFlow Lite (INT8)..."
radae-convert-tflite "$OUTPUT_DIR/onnx" "$OUTPUT_DIR/tflite" \
    --quantize int8 \
    --calibration-samples 100

# Step 3: Verify accuracy
echo ""
echo "Step 3/3: Verifying converted model accuracy..."
radae-verify-accuracy "$CHECKPOINT" "$OUTPUT_DIR/tflite" \
    --tolerance 0.01 \
    --test-samples 50

echo ""
echo "Conversion complete! Models saved to $OUTPUT_DIR"
echo "  - ONNX models: $OUTPUT_DIR/onnx/"
echo "  - TFLite models: $OUTPUT_DIR/tflite/"
EOF
    chmod 0755 ${D}${bindir}/radae-full-conversion
}

FILES:${PN} = " \
    ${bindir}/radae-export-models \
    ${bindir}/radae-convert-tflite \
    ${bindir}/radae-verify-accuracy \
    ${bindir}/radae-full-conversion \
    ${bindir}/radae-converter/*.py \
    ${datadir}/radae/converter/*.py \
"

# Allow QA warnings for now
INSANE_SKIP:${PN} += "already-stripped"
