#!/usr/bin/env python3
"""
Convert ONNX models to TensorFlow Lite with INT8 quantization

This script converts ONNX models to TensorFlow Lite format with INT8
quantization for NPU deployment on i.MX 8M Plus.

Copyright (c) 2024 David Rowe
License: MIT
"""

import os
import sys
import argparse
import numpy as np


def onnx_to_tensorflow(onnx_path, tf_output_dir):
    """
    Convert ONNX model to TensorFlow SavedModel format

    Args:
        onnx_path: Path to ONNX model
        tf_output_dir: Output directory for TensorFlow SavedModel

    Returns:
        True if conversion succeeded
    """
    try:
        import onnx
        from onnx_tf.backend import prepare

        print(f"Converting ONNX to TensorFlow: {onnx_path}")

        # Load ONNX model
        onnx_model = onnx.load(onnx_path)

        # Convert to TensorFlow
        tf_rep = prepare(onnx_model)

        # Export as SavedModel
        tf_rep.export_graph(tf_output_dir)

        print(f"  ✓ TensorFlow SavedModel created: {tf_output_dir}")
        return True

    except Exception as e:
        print(f"  ✗ ONNX to TensorFlow conversion failed: {e}")
        # Try alternative method using tf2onnx
        return onnx_to_tensorflow_tf2onnx(onnx_path, tf_output_dir)


def onnx_to_tensorflow_tf2onnx(onnx_path, tf_output_dir):
    """
    Alternative conversion using tf2onnx (reverse direction)

    This is a workaround - we actually need onnx-tensorflow
    """
    print("  Trying alternative conversion method...")
    # This would require a different approach
    return False


def load_calibration_data(calibration_file, num_samples=100):
    """
    Load calibration data for quantization

    Args:
        calibration_file: Path to .f32 feature file
        num_samples: Number of samples to use

    Returns:
        Calibration dataset
    """
    print(f"Loading calibration data: {calibration_file}")

    try:
        # Load features from .f32 file
        features = np.fromfile(calibration_file, dtype=np.float32)

        # Reshape based on expected input
        # For encoder: [batch, 4, 20]
        # For decoder: [batch, 1, 80]
        # Auto-detect based on size
        if len(features) % 80 == 0:
            # Decoder calibration
            num_vectors = min(num_samples, len(features) // 80)
            features = features[:num_vectors * 80].reshape(-1, 1, 80)
        else:
            # Encoder calibration (20 features)
            num_vectors = min(num_samples, len(features) // 20)
            features = features[:num_vectors * 20].reshape(-1, 1, 20)

        print(f"  ✓ Loaded {features.shape[0]} calibration samples")
        print(f"  Shape: {features.shape}")

        return features

    except Exception as e:
        print(f"  ✗ Failed to load calibration data: {e}")
        # Return dummy data
        print("  Using dummy calibration data")
        return np.random.randn(num_samples, 4, 20).astype(np.float32)


def representative_dataset_generator(calibration_data):
    """
    Generator for representative dataset (required for INT8 quantization)

    Args:
        calibration_data: Numpy array of calibration samples

    Yields:
        Individual samples for quantization
    """
    for sample in calibration_data:
        # Expand dims for batch dimension
        yield [np.expand_dims(sample, axis=0).astype(np.float32)]


def convert_to_tflite(saved_model_dir, output_path, quantize='int8',
                      calibration_data=None, optimize_for_size=True):
    """
    Convert TensorFlow SavedModel to TFLite with quantization

    Args:
        saved_model_dir: Path to TensorFlow SavedModel directory
        output_path: Output path for TFLite model
        quantize: Quantization mode ('int8', 'float16', 'dynamic', or None)
        calibration_data: Calibration data for INT8 quantization
        optimize_for_size: Optimize for size vs speed

    Returns:
        True if conversion succeeded
    """
    try:
        import tensorflow as tf

        print(f"Converting to TensorFlow Lite: {output_path}")
        print(f"  Quantization: {quantize}")

        # Create TFLite converter
        converter = tf.lite.TFLiteConverter.from_saved_model(saved_model_dir)

        # Set optimization flags
        if quantize == 'int8':
            # Full integer quantization
            converter.optimizations = [tf.lite.Optimize.DEFAULT]
            converter.target_spec.supported_ops = [
                tf.lite.OpsSet.TFLITE_BUILTINS_INT8
            ]

            # Set representative dataset for calibration
            if calibration_data is not None:
                converter.representative_dataset = lambda: representative_dataset_generator(
                    calibration_data
                )

            # Enforce INT8 for all tensors
            converter.inference_input_type = tf.int8
            converter.inference_output_type = tf.int8

            print(f"  Using INT8 quantization with {len(calibration_data)} calibration samples")

        elif quantize == 'float16':
            # Float16 quantization
            converter.optimizations = [tf.lite.Optimize.DEFAULT]
            converter.target_spec.supported_types = [tf.float16]
            print(f"  Using FLOAT16 quantization")

        elif quantize == 'dynamic':
            # Dynamic range quantization
            converter.optimizations = [tf.lite.Optimize.DEFAULT]
            print(f"  Using dynamic range quantization")

        else:
            print(f"  No quantization")

        # Additional optimizations for NPU
        if optimize_for_size:
            converter.optimizations = [tf.lite.Optimize.OPTIMIZE_FOR_SIZE]

        # Convert model
        tflite_model = converter.convert()

        # Save TFLite model
        with open(output_path, 'wb') as f:
            f.write(tflite_model)

        # Print model info
        model_size = len(tflite_model) / 1024  # KB
        print(f"  ✓ TFLite model created")
        print(f"  Size: {model_size:.2f} KB")

        return True

    except Exception as e:
        print(f"  ✗ TFLite conversion failed: {e}")
        import traceback
        traceback.print_exc()
        return False


def verify_tflite_model(tflite_path, input_shape):
    """
    Verify TFLite model can be loaded and run

    Args:
        tflite_path: Path to TFLite model
        input_shape: Expected input shape

    Returns:
        True if verification passed
    """
    try:
        import tensorflow as tf

        print(f"Verifying TFLite model: {tflite_path}")

        # Load TFLite model
        interpreter = tf.lite.Interpreter(model_path=tflite_path)
        interpreter.allocate_tensors()

        # Get input and output details
        input_details = interpreter.get_input_details()
        output_details = interpreter.get_output_details()

        print(f"  Input details:")
        print(f"    Shape: {input_details[0]['shape']}")
        print(f"    Type: {input_details[0]['dtype']}")

        print(f"  Output details:")
        print(f"    Shape: {output_details[0]['shape']}")
        print(f"    Type: {output_details[0]['dtype']}")

        # Test with dummy input
        input_data = np.random.randn(*input_details[0]['shape']).astype(
            input_details[0]['dtype']
        )

        interpreter.set_tensor(input_details[0]['index'], input_data)
        interpreter.invoke()

        output_data = interpreter.get_tensor(output_details[0]['index'])

        print(f"  ✓ Model verification passed")
        print(f"  Output shape: {output_data.shape}")

        return True

    except Exception as e:
        print(f"  ✗ Verification failed: {e}")
        return False


def main():
    parser = argparse.ArgumentParser(
        description='Convert ONNX models to TensorFlow Lite with quantization'
    )
    parser.add_argument(
        'onnx_model',
        type=str,
        help='Path to ONNX model file'
    )
    parser.add_argument(
        'output_tflite',
        type=str,
        help='Output path for TFLite model'
    )
    parser.add_argument(
        '--quantize',
        choices=['int8', 'float16', 'dynamic', 'none'],
        default='int8',
        help='Quantization mode (default: int8)'
    )
    parser.add_argument(
        '--calibration-data',
        type=str,
        help='Path to calibration data (.f32 file) for INT8 quantization'
    )
    parser.add_argument(
        '--calibration-samples',
        type=int,
        default=100,
        help='Number of calibration samples (default: 100)'
    )
    parser.add_argument(
        '--input-shape',
        type=str,
        help='Input shape (e.g., "1,4,20" for encoder)'
    )
    parser.add_argument(
        '--verify',
        action='store_true',
        default=True,
        help='Verify TFLite model after conversion (default: True)'
    )
    parser.add_argument(
        '--optimize-size',
        action='store_true',
        default=True,
        help='Optimize for size (default: True)'
    )

    args = parser.parse_args()

    print("=" * 60)
    print("ONNX to TensorFlow Lite Conversion")
    print("=" * 60)
    print(f"ONNX model: {args.onnx_model}")
    print(f"Output TFLite: {args.output_tflite}")
    print(f"Quantization: {args.quantize}")
    print()

    # Parse input shape
    input_shape = None
    if args.input_shape:
        input_shape = tuple(map(int, args.input_shape.split(',')))

    # Create temporary directory for TensorFlow SavedModel
    import tempfile
    with tempfile.TemporaryDirectory() as tmpdir:
        tf_model_dir = os.path.join(tmpdir, 'tf_model')

        # Step 1: Convert ONNX to TensorFlow SavedModel
        print("Step 1: Converting ONNX to TensorFlow SavedModel")
        if not onnx_to_tensorflow(args.onnx_model, tf_model_dir):
            print("✗ Conversion failed")
            return 1
        print()

        # Step 2: Load calibration data if needed
        calibration_data = None
        if args.quantize == 'int8':
            if args.calibration_data:
                calibration_data = load_calibration_data(
                    args.calibration_data,
                    args.calibration_samples
                )
            else:
                print("Warning: No calibration data provided for INT8 quantization")
                print("Using random calibration data (not recommended)")
                # Use default shape for encoder
                calibration_data = np.random.randn(
                    args.calibration_samples, 4, 20
                ).astype(np.float32)
            print()

        # Step 3: Convert to TFLite with quantization
        print("Step 2: Converting to TensorFlow Lite")
        quantize_mode = None if args.quantize == 'none' else args.quantize
        if not convert_to_tflite(
            tf_model_dir,
            args.output_tflite,
            quantize=quantize_mode,
            calibration_data=calibration_data,
            optimize_for_size=args.optimize_size
        ):
            print("✗ Conversion failed")
            return 1
        print()

        # Step 4: Verify TFLite model
        if args.verify:
            print("Step 3: Verifying TFLite model")
            verify_tflite_model(args.output_tflite, input_shape)
            print()

    print("=" * 60)
    print("Conversion completed successfully!")
    print(f"TFLite model saved to: {args.output_tflite}")
    print("=" * 60)

    return 0


if __name__ == '__main__':
    sys.exit(main())
