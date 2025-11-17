SUMMARY = "RADAE Python Package"
DESCRIPTION = "Python module for Radio Autoencoder (RADAE) neural vocoder with PyTorch backend"
HOMEPAGE = "https://github.com/drowe67/radae"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

# Dependencies
DEPENDS = "python3 python3-pip-native python3-setuptools-native python3-wheel-native"

# Runtime Python dependencies
RDEPENDS:${PN} = " \
    python3-core \
    python3-numpy \
    python3-torch \
    python3-modules \
"

# Source from local repository
SRC_URI = " \
    file://setup.py \
    file://radae/__init__.py \
    file://radae/radae.py \
    file://radae/radae_base.py \
    file://radae/bbfm.py \
    file://radae/dsp.py \
    file://radae/dataset.py \
    file://loss.py \
    file://firtorch.py \
"

S = "${WORKDIR}"

inherit setuptools3

# Native build support for conversion tools
BBCLASSEXTEND = "native nativesdk"

do_configure:prepend() {
    # Generate setup.py if not provided
    if [ ! -f "${S}/setup.py" ]; then
        cat > ${S}/setup.py << 'EOF'
#!/usr/bin/env python3
from setuptools import setup, find_packages

with open("README.md", "r", encoding="utf-8") as fh:
    long_description = fh.read()

setup(
    name="radae",
    version="1.0.0",
    author="David Rowe",
    author_email="david@rowetel.com",
    description="Radio Autoencoder (RADAE) Neural Vocoder",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/drowe67/radae",
    packages=find_packages(),
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
        "Topic :: Scientific/Engineering :: Artificial Intelligence",
        "Topic :: Communications :: Ham Radio",
    ],
    python_requires=">=3.7",
    install_requires=[
        "numpy>=1.19.0",
        "torch>=1.9.0",
    ],
    extras_require={
        "dev": [
            "pytest>=6.0",
            "black>=21.0",
        ],
        "export": [
            "onnx>=1.10.0",
            "onnxruntime>=1.9.0",
        ],
    },
)
EOF
    fi

    # Generate README if not provided
    if [ ! -f "${S}/README.md" ]; then
        cat > ${S}/README.md << 'EOF'
# RADAE - Radio Autoencoder

Neural vocoder for radio communications using rate-distortion optimized variational autoencoders.

## Features

- Encoder-decoder architecture for speech compression
- Optimized for HF radio channels
- Support for multipath and fading
- PyTorch implementation
- TensorFlow Lite export for embedded deployment

## Installation

```bash
pip install radae
```

## Usage

```python
from radae import RADAE

# Load model
model = RADAE(num_features=20, latent_dim=80, EbNodB=10.0)
model.load_checkpoint('checkpoint.pth')

# Encode features
z = model.core_encoder(features)

# Decode features
features_hat = model.core_decoder(z)
```

## License

MIT License - See LICENSE file for details
EOF
    fi
}

do_compile() {
    # Build Python wheel
    ${STAGING_BINDIR_NATIVE}/python3-native/python3 setup.py bdist_wheel
}

do_install() {
    # Install Python package
    install -d ${D}${PYTHON_SITEPACKAGES_DIR}

    # Use pip to install the wheel
    ${STAGING_BINDIR_NATIVE}/pip3 install \
        --no-deps \
        --target=${D}${PYTHON_SITEPACKAGES_DIR} \
        --no-cache-dir \
        --disable-pip-version-check \
        dist/*.whl || \
    # Fallback to setuptools install
    ${STAGING_BINDIR_NATIVE}/python3-native/python3 setup.py install \
        --root=${D} \
        --prefix=${prefix} \
        --install-lib=${PYTHON_SITEPACKAGES_DIR}

    # Install additional utilities
    install -d ${D}${bindir}

    # Create Python module test script
    cat > ${D}${bindir}/radae-test << 'EOF'
#!/usr/bin/env python3
"""Test RADAE Python module installation"""
import sys

def test_import():
    """Test basic module imports"""
    try:
        import radae
        print(f"✓ radae module imported successfully")
        print(f"  Version: {radae.__version__ if hasattr(radae, '__version__') else 'unknown'}")

        from radae import RADAE
        print(f"✓ RADAE class available")

        import torch
        print(f"✓ PyTorch available (version {torch.__version__})")

        import numpy
        print(f"✓ NumPy available (version {numpy.__version__})")

        return True
    except ImportError as e:
        print(f"✗ Import failed: {e}", file=sys.stderr)
        return False

def test_model_creation():
    """Test creating a RADAE model instance"""
    try:
        from radae import RADAE
        import torch

        model = RADAE(num_features=20, latent_dim=80, EbNodB=10.0)
        print(f"✓ RADAE model created")
        print(f"  Features: {model.feature_dim}")
        print(f"  Latent dim: {model.latent_dim}")

        # Test with dummy input
        features = torch.randn(1, 4, 20)
        z = model.core_encoder(features)
        print(f"✓ Encoder forward pass successful")
        print(f"  Input shape: {features.shape}")
        print(f"  Output shape: {z.shape}")

        features_hat = model.core_decoder(z)
        print(f"✓ Decoder forward pass successful")
        print(f"  Output shape: {features_hat.shape}")

        return True
    except Exception as e:
        print(f"✗ Model creation failed: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    print("Testing RADAE Python module...")
    print("=" * 50)

    success = True
    success &= test_import()
    print()
    success &= test_model_creation()

    print("=" * 50)
    if success:
        print("All tests passed!")
        sys.exit(0)
    else:
        print("Some tests failed!")
        sys.exit(1)
EOF
    chmod 0755 ${D}${bindir}/radae-test
}

# Unit tests
do_install_ptest() {
    install -d ${D}${PTEST_PATH}

    cat > ${D}${PTEST_PATH}/run-ptest << 'EOF'
#!/bin/sh
# Run RADAE Python module tests
radae-test
EOF
    chmod 0755 ${D}${PTEST_PATH}/run-ptest
}

FILES:${PN} = " \
    ${PYTHON_SITEPACKAGES_DIR}/radae/* \
    ${PYTHON_SITEPACKAGES_DIR}/radae-*.egg-info/* \
    ${bindir}/radae-test \
"

FILES:${PN}-ptest = "${PTEST_PATH}/*"

# Development files
FILES:${PN}-dev = " \
    ${PYTHON_SITEPACKAGES_DIR}/radae/*.h \
"

RDEPENDS:${PN}-ptest = "${PN}"
