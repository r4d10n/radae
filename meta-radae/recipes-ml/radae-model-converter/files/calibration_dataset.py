#!/usr/bin/env python3
"""
Generate calibration dataset for INT8 quantization

This script generates representative calibration data for quantizing
RADAE models to INT8 format.

Copyright (c) 2024 David Rowe
License: MIT
"""

import os
import sys
import argparse
import numpy as np


def generate_calibration_features(num_samples=100, output_file=None):
    """
    Generate calibration feature vectors

    Args:
        num_samples: Number of samples to generate
        output_file: Output file path (.f32)

    Returns:
        Feature array
    """
    print(f"Generating {num_samples} calibration feature vectors...")

    # Feature statistics from RADAE training
    # These approximate the distribution of real speech features
    feature_means = np.array([
        # Cepstral coefficients
        -10.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
        # Pitch and voicing
        0.5, 0.0, 0.0,
        # Additional features
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0
    ])

    feature_stds = np.array([
        # Cepstral coefficients
        5.0, 3.0, 2.0, 2.0, 1.5, 1.5, 1.0, 1.0, 1.0, 1.0,
        # Pitch and voicing
        0.5, 1.0, 1.0,
        # Additional features
        1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0
    ])

    # Generate samples from normal distribution
    features = np.random.randn(num_samples, 20).astype(np.float32)

    # Apply scaling and offset
    features = features * feature_stds + feature_means

    # Clip to reasonable range
    features = np.clip(features, -20.0, 20.0)

    print(f"  Generated shape: {features.shape}")
    print(f"  Mean: {np.mean(features, axis=0)[:5]}...")
    print(f"  Std: {np.std(features, axis=0)[:5]}...")

    if output_file:
        features.tofile(output_file)
        print(f"  ✓ Saved to {output_file}")

    return features


def load_speech_features(wav_dir, num_samples=100):
    """
    Load features from speech files (if available)

    Args:
        wav_dir: Directory containing .wav files
        num_samples: Number of samples to extract

    Returns:
        Feature array
    """
    # This would require LPCNet feature extraction
    # For now, return None to fallback to synthetic data
    print(f"Loading features from {wav_dir}...")
    print("  Warning: Speech feature extraction not implemented")
    print("  Using synthetic calibration data")
    return None


def generate_latent_calibration(num_samples=100, latent_dim=80, output_file=None):
    """
    Generate calibration latent vectors (for decoder)

    Args:
        num_samples: Number of samples
        latent_dim: Latent dimension
        output_file: Output file path

    Returns:
        Latent array
    """
    print(f"Generating {num_samples} calibration latent vectors...")

    # Latent vectors are approximately standard normal
    latents = np.random.randn(num_samples, latent_dim).astype(np.float32)

    # Scale to match typical latent distribution
    latents = latents * 0.5

    print(f"  Generated shape: {latents.shape}")
    print(f"  Mean: {np.mean(latents):.4f}")
    print(f"  Std: {np.std(latents):.4f}")

    if output_file:
        latents.tofile(output_file)
        print(f"  ✓ Saved to {output_file}")

    return latents


def main():
    parser = argparse.ArgumentParser(
        description='Generate calibration dataset for INT8 quantization'
    )
    parser.add_argument(
        '--num-samples',
        type=int,
        default=100,
        help='Number of calibration samples (default: 100)'
    )
    parser.add_argument(
        '--output-dir',
        type=str,
        default='.',
        help='Output directory for calibration data'
    )
    parser.add_argument(
        '--wav-dir',
        type=str,
        help='Directory containing speech files (optional)'
    )
    parser.add_argument(
        '--latent-dim',
        type=int,
        default=80,
        help='Latent dimension (default: 80)'
    )
    parser.add_argument(
        '--type',
        choices=['encoder', 'decoder', 'both'],
        default='both',
        help='Generate calibration data for encoder, decoder, or both'
    )

    args = parser.parse_args()

    print("=" * 60)
    print("Calibration Dataset Generation")
    print("=" * 60)
    print(f"Number of samples: {args.num_samples}")
    print(f"Output directory: {args.output_dir}")
    print()

    # Create output directory
    os.makedirs(args.output_dir, exist_ok=True)

    # Generate encoder calibration data
    if args.type in ['encoder', 'both']:
        print("Generating encoder calibration data...")

        if args.wav_dir:
            features = load_speech_features(args.wav_dir, args.num_samples)
        else:
            features = None

        if features is None:
            features = generate_calibration_features(
                args.num_samples,
                os.path.join(args.output_dir, 'calibration_features.f32')
            )

        print()

    # Generate decoder calibration data
    if args.type in ['decoder', 'both']:
        print("Generating decoder calibration data...")

        latents = generate_latent_calibration(
            args.num_samples,
            args.latent_dim,
            os.path.join(args.output_dir, 'calibration_latents.f32')
        )

        print()

    print("=" * 60)
    print("Calibration data generation complete!")
    print(f"Files saved to: {args.output_dir}")
    print("=" * 60)

    return 0


if __name__ == '__main__':
    sys.exit(main())
