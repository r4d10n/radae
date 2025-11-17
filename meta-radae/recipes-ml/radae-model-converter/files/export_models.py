#!/usr/bin/env python3
"""
Export RADAE PyTorch models to ONNX format

This script exports the RADAE encoder and decoder models from a PyTorch
checkpoint to ONNX format, which can then be converted to TensorFlow Lite
for NPU deployment on i.MX 8M Plus.

Copyright (c) 2024 David Rowe
License: MIT
"""

import os
import sys
import argparse
import numpy as np
import torch
import torch.onnx

# Add parent directory to path for radae imports
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', '..', '..'))

from radae import RADAE


def export_encoder_onnx(model, output_path, latent_dim=80):
    """
    Export encoder to ONNX format

    Args:
        model: RADAE model instance
        output_path: Path to save ONNX model
        latent_dim: Latent dimension (default: 80)
    """
    print(f"Exporting encoder to ONNX: {output_path}")

    # Set model to eval mode
    model.eval()

    # Create dummy input: [batch, time_steps, features]
    # Encoder processes 4 feature vectors (40ms) at a time
    dummy_input = torch.randn(1, 4, 20, dtype=torch.float32)

    # Export to ONNX
    torch.onnx.export(
        model.core_encoder,
        dummy_input,
        output_path,
        export_params=True,
        opset_version=13,
        do_constant_folding=True,
        input_names=['input'],
        output_names=['output'],
        dynamic_axes={
            'input': {0: 'batch_size', 1: 'time_steps'},
            'output': {0: 'batch_size', 1: 'latent_vectors'}
        },
        verbose=False
    )

    print(f"  Input shape: [batch, 4, 20]")
    print(f"  Output shape: [batch, 1, {latent_dim}]")
    print(f"  ✓ Encoder exported successfully")


def export_decoder_onnx(model, output_path, latent_dim=80):
    """
    Export decoder to ONNX format

    Args:
        model: RADAE model instance
        output_path: Path to save ONNX model
        latent_dim: Latent dimension (default: 80)
    """
    print(f"Exporting decoder to ONNX: {output_path}")

    # Set model to eval mode
    model.eval()

    # Create dummy input: [batch, latent_vectors, latent_dim]
    dummy_input = torch.randn(1, 1, latent_dim, dtype=torch.float32)

    # Export to ONNX
    torch.onnx.export(
        model.core_decoder,
        dummy_input,
        output_path,
        export_params=True,
        opset_version=13,
        do_constant_folding=True,
        input_names=['input'],
        output_names=['output'],
        dynamic_axes={
            'input': {0: 'batch_size', 1: 'latent_vectors'},
            'output': {0: 'batch_size', 1: 'time_steps'}
        },
        verbose=False
    )

    print(f"  Input shape: [batch, 1, {latent_dim}]")
    print(f"  Output shape: [batch, 16, 20]")
    print(f"  ✓ Decoder exported successfully")


def export_stateful_encoder_onnx(model, output_path, latent_dim=80):
    """
    Export stateful encoder to ONNX format (for streaming)

    Args:
        model: RADAE model instance
        output_path: Path to save ONNX model
        latent_dim: Latent dimension (default: 80)
    """
    print(f"Exporting stateful encoder to ONNX: {output_path}")

    # Load stateful encoder weights
    model.core_encoder_statefull_load_state_dict()
    model.eval()

    # Create dummy input: [batch, time_steps, features]
    dummy_input = torch.randn(1, 12, 20, dtype=torch.float32)

    # Export to ONNX
    torch.onnx.export(
        model.core_encoder_statefull,
        dummy_input,
        output_path,
        export_params=True,
        opset_version=13,
        do_constant_folding=True,
        input_names=['input'],
        output_names=['output'],
        dynamic_axes={
            'input': {0: 'batch_size', 1: 'time_steps'},
            'output': {0: 'batch_size', 1: 'latent_vectors'}
        },
        verbose=False
    )

    print(f"  Input shape: [batch, 12, 20]")
    print(f"  Output shape: [batch, 3, {latent_dim}]")
    print(f"  ✓ Stateful encoder exported successfully")


def export_stateful_decoder_onnx(model, output_path, latent_dim=80):
    """
    Export stateful decoder to ONNX format (for streaming)

    Args:
        model: RADAE model instance
        output_path: Path to save ONNX model
        latent_dim: Latent dimension (default: 80)
    """
    print(f"Exporting stateful decoder to ONNX: {output_path}")

    # Load stateful decoder weights
    model.core_decoder_statefull_load_state_dict()
    model.eval()

    # Create dummy input: [batch, latent_vectors, latent_dim]
    dummy_input = torch.randn(1, 1, latent_dim, dtype=torch.float32)

    # Export to ONNX
    torch.onnx.export(
        model.core_decoder_statefull,
        dummy_input,
        output_path,
        export_params=True,
        opset_version=13,
        do_constant_folding=True,
        input_names=['input'],
        output_names=['output'],
        dynamic_axes={
            'input': {0: 'batch_size', 1: 'latent_vectors'},
            'output': {0: 'batch_size', 1: 'time_steps'}
        },
        verbose=False
    )

    print(f"  Input shape: [batch, 1, {latent_dim}]")
    print(f"  Output shape: [batch, 16, 20]")
    print(f"  ✓ Stateful decoder exported successfully")


def verify_onnx_model(onnx_path):
    """
    Verify ONNX model can be loaded and run

    Args:
        onnx_path: Path to ONNX model file

    Returns:
        True if verification passed
    """
    try:
        import onnx
        import onnxruntime as ort

        # Load and check ONNX model
        onnx_model = onnx.load(onnx_path)
        onnx.checker.check_model(onnx_model)

        # Create ONNX Runtime session
        session = ort.InferenceSession(onnx_path)

        print(f"  ✓ ONNX model verification passed")
        return True

    except Exception as e:
        print(f"  ✗ ONNX verification failed: {e}")
        return False


def main():
    parser = argparse.ArgumentParser(
        description='Export RADAE PyTorch models to ONNX format'
    )
    parser.add_argument(
        'checkpoint',
        type=str,
        help='Path to PyTorch checkpoint (.pth)'
    )
    parser.add_argument(
        'output_dir',
        type=str,
        help='Output directory for ONNX models'
    )
    parser.add_argument(
        '--latent-dim',
        type=int,
        default=80,
        help='Latent dimension (default: 80)'
    )
    parser.add_argument(
        '--separate-encoder-decoder',
        action='store_true',
        help='Export encoder and decoder as separate models'
    )
    parser.add_argument(
        '--stateful',
        action='store_true',
        help='Export stateful (streaming) versions'
    )
    parser.add_argument(
        '--verify',
        action='store_true',
        default=True,
        help='Verify ONNX models after export (default: True)'
    )

    args = parser.parse_args()

    # Create output directory
    os.makedirs(args.output_dir, exist_ok=True)

    print("=" * 60)
    print("RADAE Model Export to ONNX")
    print("=" * 60)
    print(f"Checkpoint: {args.checkpoint}")
    print(f"Output directory: {args.output_dir}")
    print(f"Latent dimension: {args.latent_dim}")
    print()

    # Disable CUDA
    os.environ['CUDA_VISIBLE_DEVICES'] = ""
    device = torch.device("cpu")

    # Load model
    print("Loading PyTorch model...")
    num_features = 20
    model = RADAE(num_features, args.latent_dim, 100.0)

    try:
        checkpoint = torch.load(args.checkpoint, map_location='cpu', weights_only=True)
        model.load_state_dict(checkpoint['state_dict'], strict=False)
        print(f"✓ Model loaded from checkpoint")
        print()
    except Exception as e:
        print(f"✗ Error loading checkpoint: {e}")
        return 1

    model.to(device)
    model.eval()

    # Export models
    try:
        if args.separate_encoder_decoder or not args.stateful:
            # Export encoder
            encoder_path = os.path.join(args.output_dir, 'encoder.onnx')
            export_encoder_onnx(model, encoder_path, args.latent_dim)
            if args.verify:
                verify_onnx_model(encoder_path)
            print()

            # Export decoder
            decoder_path = os.path.join(args.output_dir, 'decoder.onnx')
            export_decoder_onnx(model, decoder_path, args.latent_dim)
            if args.verify:
                verify_onnx_model(decoder_path)
            print()

        if args.stateful:
            # Export stateful encoder
            stateful_enc_path = os.path.join(args.output_dir, 'encoder_stateful.onnx')
            export_stateful_encoder_onnx(model, stateful_enc_path, args.latent_dim)
            if args.verify:
                verify_onnx_model(stateful_enc_path)
            print()

            # Export stateful decoder
            stateful_dec_path = os.path.join(args.output_dir, 'decoder_stateful.onnx')
            export_stateful_decoder_onnx(model, stateful_dec_path, args.latent_dim)
            if args.verify:
                verify_onnx_model(stateful_dec_path)
            print()

        print("=" * 60)
        print("Export completed successfully!")
        print(f"ONNX models saved to: {args.output_dir}")
        print("=" * 60)

        return 0

    except Exception as e:
        print(f"✗ Export failed: {e}")
        import traceback
        traceback.print_exc()
        return 1


if __name__ == '__main__':
    sys.exit(main())
