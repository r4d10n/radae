#!/usr/bin/env python3
"""Generate test vector data for RADAE NPU tests"""

import struct
import math
import random

def generate_sine_wave(filename, freq=440.0, duration=1.0, sample_rate=8000):
    """Generate a sine wave test signal"""
    num_samples = int(sample_rate * duration)

    with open(filename, 'wb') as f:
        for i in range(num_samples):
            t = i / sample_rate
            val = 0.5 * math.sin(2.0 * math.pi * freq * t)
            sample = int(val * 32767)
            # Write as 16-bit signed integer (little endian)
            f.write(struct.pack('<h', sample))

    print(f"Generated {filename}: {num_samples} samples, {duration}s @ {sample_rate}Hz")

def generate_expected_features(filename, feature_dim=20, num_frames=50):
    """Generate expected feature vectors (simulated)"""

    with open(filename, 'wb') as f:
        for frame in range(num_frames):
            for feat in range(feature_dim):
                # Generate synthetic feature values
                val = random.gauss(0.0, 1.0)
                f.write(struct.pack('<f', val))

    print(f"Generated {filename}: {num_frames} frames x {feature_dim} features")

if __name__ == '__main__':
    print("Generating RADAE NPU test vectors...")

    # Generate speech-like test signal (1 second at 8kHz)
    generate_sine_wave('speech_8k.raw', freq=440.0, duration=1.0, sample_rate=8000)

    # Generate expected features
    generate_expected_features('expected_features.bin', feature_dim=20, num_frames=50)

    print("Test vectors generated successfully!")
