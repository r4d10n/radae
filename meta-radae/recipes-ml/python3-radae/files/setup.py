#!/usr/bin/env python3
"""
Setup script for RADAE Python package

Copyright (c) 2024 David Rowe
License: MIT
"""

from setuptools import setup, find_packages
import os

# Read README if it exists
readme_file = os.path.join(os.path.dirname(__file__), "README.md")
if os.path.exists(readme_file):
    with open(readme_file, "r", encoding="utf-8") as fh:
        long_description = fh.read()
else:
    long_description = "Radio Autoencoder (RADAE) Neural Vocoder"

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
        "Programming Language :: Python :: 3.7",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
        "Topic :: Scientific/Engineering :: Artificial Intelligence",
        "Topic :: Communications :: Ham Radio",
        "Intended Audience :: Developers",
        "Intended Audience :: Science/Research",
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
            "flake8>=3.9",
        ],
        "export": [
            "onnx>=1.10.0",
            "onnxruntime>=1.9.0",
            "tensorflow-lite>=2.8.0",
        ],
    },
    package_data={
        "radae": ["*.py"],
    },
    include_package_data=True,
)
