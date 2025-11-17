#!/usr/bin/env python3
"""
Quantization utilities for RADAE models

Helper functions for quantizing models to different precisions.

Copyright (c) 2024 David Rowe
License: MIT
"""

import numpy as np


def quantize_int8(values, scale=None, zero_point=None):
    """
    Quantize float values to INT8

    Args:
        values: Float array to quantize
        scale: Quantization scale (computed if None)
        zero_point: Zero point (computed if None)

    Returns:
        Quantized values, scale, zero_point
    """
    if scale is None or zero_point is None:
        # Compute scale and zero point
        min_val = np.min(values)
        max_val = np.max(values)

        # INT8 range: -128 to 127
        qmin = -128
        qmax = 127

        scale = (max_val - min_val) / (qmax - qmin)
        zero_point = qmin - min_val / scale

        zero_point = np.clip(np.round(zero_point), qmin, qmax).astype(np.int8)

    # Quantize
    quantized = np.clip(
        np.round(values / scale + zero_point),
        -128, 127
    ).astype(np.int8)

    return quantized, scale, zero_point


def dequantize_int8(quantized, scale, zero_point):
    """
    Dequantize INT8 values back to float

    Args:
        quantized: Quantized INT8 array
        scale: Quantization scale
        zero_point: Zero point

    Returns:
        Dequantized float array
    """
    return (quantized.astype(np.float32) - zero_point) * scale


def compute_quantization_error(original, quantized):
    """
    Compute quantization error metrics

    Args:
        original: Original float values
        quantized: Quantized and dequantized values

    Returns:
        Dictionary of error metrics
    """
    mse = np.mean((original - quantized) ** 2)
    mae = np.mean(np.abs(original - quantized))
    max_error = np.max(np.abs(original - quantized))

    # SNR
    signal_power = np.mean(original ** 2)
    noise_power = np.mean((original - quantized) ** 2)
    snr_db = 10 * np.log10(signal_power / (noise_power + 1e-10))

    return {
        'mse': mse,
        'mae': mae,
        'max_error': max_error,
        'snr_db': snr_db
    }


def analyze_quantization_impact(model_weights, num_samples=1000):
    """
    Analyze the impact of INT8 quantization on model weights

    Args:
        model_weights: Dictionary of model weight arrays
        num_samples: Number of samples for testing

    Returns:
        Analysis results
    """
    results = {}

    for name, weights in model_weights.items():
        print(f"Analyzing {name}...")

        # Quantize and dequantize
        quantized, scale, zero_point = quantize_int8(weights)
        dequantized = dequantize_int8(quantized, scale, zero_point)

        # Compute errors
        errors = compute_quantization_error(weights, dequantized)

        results[name] = {
            'shape': weights.shape,
            'original_range': (np.min(weights), np.max(weights)),
            'scale': scale,
            'zero_point': zero_point,
            'errors': errors
        }

        print(f"  Shape: {weights.shape}")
        print(f"  Range: [{np.min(weights):.4f}, {np.max(weights):.4f}]")
        print(f"  Scale: {scale:.6f}")
        print(f"  MAE: {errors['mae']:.6f}")
        print(f"  SNR: {errors['snr_db']:.2f} dB")
        print()

    return results


if __name__ == '__main__':
    # Test quantization
    print("Testing INT8 quantization...")

    # Generate test data
    test_data = np.random.randn(1000).astype(np.float32)

    # Quantize
    quantized, scale, zero_point = quantize_int8(test_data)
    dequantized = dequantize_int8(quantized, scale, zero_point)

    # Compute error
    errors = compute_quantization_error(test_data, dequantized)

    print(f"Test data range: [{np.min(test_data):.4f}, {np.max(test_data):.4f}]")
    print(f"Scale: {scale:.6f}")
    print(f"Zero point: {zero_point}")
    print(f"MSE: {errors['mse']:.6f}")
    print(f"MAE: {errors['mae']:.6f}")
    print(f"Max error: {errors['max_error']:.6f}")
    print(f"SNR: {errors['snr_db']:.2f} dB")
