#!/usr/bin/env python3
"""
Verify accuracy of converted TensorFlow Lite models

This script compares the output of TFLite models against the original
PyTorch models to ensure conversion quality.

Copyright (c) 2024 David Rowe
License: MIT
"""

import os
import sys
import argparse
import numpy as np
import torch

# Add parent directory to path for radae imports
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', '..', '..'))

from radae import RADAE, distortion_loss


def load_pytorch_model(checkpoint_path, latent_dim=80):
    """
    Load PyTorch RADAE model from checkpoint

    Args:
        checkpoint_path: Path to PyTorch checkpoint
        latent_dim: Latent dimension

    Returns:
        Loaded model
    """
    print(f"Loading PyTorch model: {checkpoint_path}")

    os.environ['CUDA_VISIBLE_DEVICES'] = ""
    device = torch.device("cpu")

    num_features = 20
    model = RADAE(num_features, latent_dim, 100.0)

    checkpoint = torch.load(checkpoint_path, map_location='cpu', weights_only=True)
    model.load_state_dict(checkpoint['state_dict'], strict=False)

    model.to(device)
    model.eval()

    print(f"  ✓ PyTorch model loaded")
    return model


def load_tflite_model(tflite_path):
    """
    Load TensorFlow Lite model

    Args:
        tflite_path: Path to TFLite model

    Returns:
        TFLite interpreter
    """
    print(f"Loading TFLite model: {tflite_path}")

    try:
        import tensorflow as tf

        interpreter = tf.lite.Interpreter(model_path=tflite_path)
        interpreter.allocate_tensors()

        # Get input and output details
        input_details = interpreter.get_input_details()
        output_details = interpreter.get_output_details()

        print(f"  ✓ TFLite model loaded")
        print(f"  Input: {input_details[0]['shape']} ({input_details[0]['dtype']})")
        print(f"  Output: {output_details[0]['shape']} ({output_details[0]['dtype']})")

        return interpreter, input_details, output_details

    except Exception as e:
        print(f"  ✗ Failed to load TFLite model: {e}")
        return None, None, None


def load_test_features(features_path, num_samples=50):
    """
    Load test features from .f32 file

    Args:
        features_path: Path to features file
        num_samples: Number of samples to load

    Returns:
        Features array
    """
    print(f"Loading test features: {features_path}")

    try:
        # Load all features
        features = np.fromfile(features_path, dtype=np.float32)

        # Reshape to [time_steps, 20] (assuming 20 features per vector)
        num_features = 20
        total_vectors = len(features) // num_features
        features = features[:total_vectors * num_features].reshape(-1, num_features)

        # Limit to requested samples
        if num_samples > 0:
            features = features[:num_samples * 4]  # 4 vectors per encoder input

        print(f"  ✓ Loaded {features.shape[0]} feature vectors")
        return features

    except Exception as e:
        print(f"  ✗ Failed to load test features: {e}")
        # Generate random test data
        print("  Using random test data")
        return np.random.randn(num_samples * 4, 20).astype(np.float32)


def run_pytorch_encoder(model, features):
    """
    Run PyTorch encoder on features

    Args:
        model: PyTorch RADAE model
        features: Input features [time_steps, features]

    Returns:
        Latent vectors
    """
    # Reshape for encoder: [batch, 4, 20]
    num_frames = len(features) // 4
    features_reshaped = features[:num_frames * 4].reshape(1, num_frames, 4, 20)

    with torch.no_grad():
        latents = []
        for i in range(num_frames):
            z = model.core_encoder(
                torch.tensor(features_reshaped[0, i:i+1, :, :], dtype=torch.float32)
            )
            latents.append(z)

        latents = torch.cat(latents, dim=1)

    return latents.cpu().numpy()


def run_tflite_encoder(interpreter, input_details, output_details, features):
    """
    Run TFLite encoder on features

    Args:
        interpreter: TFLite interpreter
        input_details: Input tensor details
        output_details: Output tensor details
        features: Input features [time_steps, features]

    Returns:
        Latent vectors
    """
    # Reshape for encoder: [batch, 4, 20]
    num_frames = len(features) // 4
    features_reshaped = features[:num_frames * 4].reshape(num_frames, 4, 20)

    latents = []
    for i in range(num_frames):
        # Prepare input
        input_data = features_reshaped[i:i+1].astype(np.float32)

        # Set input tensor
        interpreter.set_tensor(input_details[0]['index'], input_data)

        # Run inference
        interpreter.invoke()

        # Get output
        output_data = interpreter.get_tensor(output_details[0]['index'])
        latents.append(output_data)

    return np.concatenate(latents, axis=0)


def compute_metrics(pytorch_output, tflite_output):
    """
    Compute accuracy metrics between PyTorch and TFLite outputs

    Args:
        pytorch_output: PyTorch model output
        tflite_output: TFLite model output

    Returns:
        Dictionary of metrics
    """
    # Ensure same shape
    min_len = min(pytorch_output.shape[0], tflite_output.shape[0])
    pytorch_output = pytorch_output[:min_len]
    tflite_output = tflite_output[:min_len]

    # Compute metrics
    mse = np.mean((pytorch_output - tflite_output) ** 2)
    mae = np.mean(np.abs(pytorch_output - tflite_output))
    max_error = np.max(np.abs(pytorch_output - tflite_output))

    # Compute correlation
    pytorch_flat = pytorch_output.flatten()
    tflite_flat = tflite_output.flatten()
    correlation = np.corrcoef(pytorch_flat, tflite_flat)[0, 1]

    # Compute SNR
    signal_power = np.mean(pytorch_output ** 2)
    noise_power = np.mean((pytorch_output - tflite_output) ** 2)
    snr_db = 10 * np.log10(signal_power / (noise_power + 1e-10))

    return {
        'mse': mse,
        'mae': mae,
        'max_error': max_error,
        'correlation': correlation,
        'snr_db': snr_db
    }


def verify_encoder(pytorch_model, tflite_dir, test_features, tolerance=0.01):
    """
    Verify encoder accuracy

    Args:
        pytorch_model: PyTorch RADAE model
        tflite_dir: Directory containing TFLite models
        test_features: Test feature data
        tolerance: Acceptable error tolerance

    Returns:
        True if accuracy is acceptable
    """
    print()
    print("=" * 60)
    print("Verifying Encoder")
    print("=" * 60)

    # Load TFLite encoder
    encoder_path = os.path.join(tflite_dir, 'encoder_int8.tflite')
    if not os.path.exists(encoder_path):
        encoder_path = os.path.join(tflite_dir, 'encoder.tflite')

    if not os.path.exists(encoder_path):
        print(f"✗ Encoder not found: {encoder_path}")
        return False

    interpreter, input_details, output_details = load_tflite_model(encoder_path)
    if interpreter is None:
        return False

    print()

    # Run PyTorch encoder
    print("Running PyTorch encoder...")
    pytorch_latents = run_pytorch_encoder(pytorch_model, test_features)
    print(f"  Output shape: {pytorch_latents.shape}")

    # Run TFLite encoder
    print("Running TFLite encoder...")
    tflite_latents = run_tflite_encoder(
        interpreter, input_details, output_details, test_features
    )
    print(f"  Output shape: {tflite_latents.shape}")

    # Compute metrics
    print()
    print("Computing accuracy metrics...")
    metrics = compute_metrics(pytorch_latents, tflite_latents)

    print(f"  MSE:         {metrics['mse']:.6f}")
    print(f"  MAE:         {metrics['mae']:.6f}")
    print(f"  Max Error:   {metrics['max_error']:.6f}")
    print(f"  Correlation: {metrics['correlation']:.6f}")
    print(f"  SNR (dB):    {metrics['snr_db']:.2f}")

    # Check if accuracy is acceptable
    passed = metrics['mae'] < tolerance and metrics['correlation'] > 0.95

    if passed:
        print(f"  ✓ Encoder accuracy: PASS (MAE={metrics['mae']:.6f} < {tolerance})")
    else:
        print(f"  ✗ Encoder accuracy: FAIL (MAE={metrics['mae']:.6f} >= {tolerance})")

    return passed


def main():
    parser = argparse.ArgumentParser(
        description='Verify accuracy of converted TFLite models'
    )
    parser.add_argument(
        'checkpoint',
        type=str,
        help='Path to PyTorch checkpoint'
    )
    parser.add_argument(
        'tflite_dir',
        type=str,
        help='Directory containing TFLite models'
    )
    parser.add_argument(
        '--test-data',
        type=str,
        help='Path to test features (.f32 file)'
    )
    parser.add_argument(
        '--test-samples',
        type=int,
        default=50,
        help='Number of test samples (default: 50)'
    )
    parser.add_argument(
        '--latent-dim',
        type=int,
        default=80,
        help='Latent dimension (default: 80)'
    )
    parser.add_argument(
        '--tolerance',
        type=float,
        default=0.02,
        help='Acceptable error tolerance (default: 0.02)'
    )

    args = parser.parse_args()

    print("=" * 60)
    print("RADAE Model Accuracy Verification")
    print("=" * 60)
    print(f"PyTorch checkpoint: {args.checkpoint}")
    print(f"TFLite directory: {args.tflite_dir}")
    print(f"Tolerance: {args.tolerance}")
    print()

    # Load PyTorch model
    pytorch_model = load_pytorch_model(args.checkpoint, args.latent_dim)

    # Load test features
    if args.test_data and os.path.exists(args.test_data):
        test_features = load_test_features(args.test_data, args.test_samples)
    else:
        print("No test data provided, generating random features")
        test_features = np.random.randn(args.test_samples * 4, 20).astype(np.float32)

    # Verify encoder
    encoder_passed = verify_encoder(
        pytorch_model, args.tflite_dir, test_features, args.tolerance
    )

    # Summary
    print()
    print("=" * 60)
    print("Verification Summary")
    print("=" * 60)
    print(f"Encoder: {'PASS' if encoder_passed else 'FAIL'}")
    print("=" * 60)

    # Note about quantization
    if not encoder_passed:
        print()
        print("Note: Some accuracy loss is expected with INT8 quantization.")
        print("Typical acceptable range: MAE < 0.05, Correlation > 0.90")
        print("For critical applications, consider using FLOAT16 quantization.")

    return 0 if encoder_passed else 1


if __name__ == '__main__':
    sys.exit(main())
