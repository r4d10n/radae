# RADAE Usage Guide - Running Different Versions

**Complete guide for converting and transmitting audio using RADAE variants**

**Date:** 2025-11-17
**RADAE Repository:** https://github.com/drowe67/radae

---

## Table of Contents

1. [Version Overview](#version-overview)
2. [Installation and Setup](#installation-and-setup)
3. [HF RADAE (Main Version)](#hf-radae-main-version)
4. [Model Variants (model05, model17, model18, model19_check3)](#model-variants)
5. [BBFM (Baseband FM for VHF/UHF)](#bbfm-baseband-fm)
6. [RADAE v2 (Experimental ML Sync)](#radae-v2-experimental)
7. [Audio Format Conversion](#audio-format-conversion)
8. [Over-the-Air Testing](#over-the-air-testing)
9. [Performance Comparison](#performance-comparison)
10. [Troubleshooting](#troubleshooting)

---

## Version Overview

### What "Versions" Actually Mean

RADAE doesn't use traditional "v1/v2" versioning. Instead:

| Name | Description | Use Case | Status |
|------|-------------|----------|--------|
| **HF RADAE** | Main implementation for HF radio (3-30 MHz) | Long-distance HF radio | **Production** |
| **model05-model19** | Different trained models with improvements | Various performance/bandwidth | **Production** |
| **model19_check3** | **Recommended production model** | Best overall performance | **Recommended** |
| **BBFM** | Baseband FM variant | VHF/UHF land mobile radio | **Production** |
| **RADEv2** | Experimental branch with ML sync/EQ | Research/development | **Experimental** |

### Key Differences

**HF RADAE (Main):**
- OFDM multi-carrier (30 carriers)
- 1500 Hz bandwidth
- Designed for HF multipath channels
- Uses pilot-based equalization
- Classical DSP sync

**BBFM:**
- Single-carrier PSK
- Baseband FM modulation
- VHF/UHF LMR use case
- Different channel model

**RADEv2:**
- ML-based sync and equalization
- Experimental fine-tuning
- Advanced channel estimation

---

## Installation and Setup

### Prerequisites

```bash
# Ubuntu 20.04/22.04 recommended
sudo apt update
sudo apt install -y \
    python3 python3-pip \
    sox \
    octave octave-signal \
    cmake build-essential

# Install PyTorch (CPU version)
pip3 install torch torchvision torchaudio --index-url https://download.pytorch.org/whl/cpu

# Install other Python dependencies
pip3 install numpy scipy matplotlib tqdm
```

### Clone and Build RADAE

```bash
# Clone repository
cd ~
git clone https://github.com/drowe67/radae.git
cd radae

# Build C components (FARGAN vocoder)
mkdir build
cd build
cmake ..
make -j$(nproc)

# Verify installation
cd ..
ls -lh model19_check3/checkpoints/checkpoint_epoch_100.pth
ls -lh build/src/lpcnet_demo
```

### Download Pre-trained Models

Models are included in the repository. Key models:

```bash
# Production model (recommended)
model19_check3/checkpoints/checkpoint_epoch_100.pth

# Reference models
model05/checkpoints/checkpoint_epoch_100.pth
model17/checkpoints/checkpoint_epoch_100.pth
model18/checkpoints/checkpoint_epoch_100.pth

# BBFM model
model_bbfm_01/checkpoints/checkpoint_epoch_100.pth
```

---

## HF RADAE (Main Version)

### Basic Transmission/Reception Pipeline

**Complete TX/RX Flow:**
```
INPUT AUDIO (WAV/MP3)
    ↓
[1] Convert to 16kHz mono WAV
    ↓
[2] Extract FARGAN features (20 features @ 100Hz)
    ↓
[3] RADAE Encoder → Latent symbols (80 dims @ 25Hz)
    ↓
[4] OFDM Modulator → IQ samples (8kHz complex)
    ↓
[5] Transmit over HF channel (or save to file)
    ↓
[6] OFDM Demodulator + Sync → Latent symbols
    ↓
[7] RADAE Decoder → FARGAN features
    ↓
[8] FARGAN Vocoder → 16kHz audio
    ↓
OUTPUT AUDIO (WAV)
```

### 1. Perfect Channel (No Noise) - Quick Test

**Transmit and receive with perfect channel:**

```bash
# Using model19_check3 (recommended)
./inference.sh model19_check3/checkpoints/checkpoint_epoch_100.pth \
    wav/brian_g8sez.wav \
    output_perfect.wav \
    --EbNodB 100

# Listen to result
aplay output_perfect.wav

# Or with explicit Python command
python3 inference.py \
    --model-path model19_check3/checkpoints/checkpoint_epoch_100.pth \
    --input wav/brian_g8sez.wav \
    --output output_perfect.wav \
    --EbNodB 100 \
    --write-latent z.f32
```

**What this does:**
- Loads model19_check3
- Processes input audio through encoder/decoder
- EbNodB=100 means virtually no noise
- Saves output to `output_perfect.wav`
- Optionally saves latent symbols to `z.f32`

### 2. AWGN Channel (Additive White Gaussian Noise)

**Test with different SNR levels:**

```bash
# High SNR (10 dB) - excellent quality
./inference.sh model19_check3/checkpoints/checkpoint_epoch_100.pth \
    wav/brian_g8sez.wav \
    output_10dB.wav \
    --EbNodB 10

# Medium SNR (6 dB) - good quality
./inference.sh model19_check3/checkpoints/checkpoint_epoch_100.pth \
    wav/brian_g8sez.wav \
    output_6dB.wav \
    --EbNodB 6

# Low SNR (3 dB) - marginal quality
./inference.sh model19_check3/checkpoints/checkpoint_epoch_100.pth \
    wav/brian_g8sez.wav \
    output_3dB.wav \
    --EbNodB 3

# Very low SNR (0 dB) - challenging
./inference.sh model19_check3/checkpoints/checkpoint_epoch_100.pth \
    wav/brian_g8sez.wav \
    output_0dB.wav \
    --EbNodB 0
```

**Compare quality:**
```bash
# Listen to outputs
for snr in perfect 10dB 6dB 3dB 0dB; do
    echo "Playing output_${snr}.wav (press Ctrl+C to skip)"
    aplay output_${snr}.wav
done
```

### 3. Multipath Channel (Realistic HF)

**Simulate multipath fading:**

```bash
# Multipath Poor (MPP) channel
python3 inference.py \
    --model-path model19_check3/checkpoints/checkpoint_epoch_100.pth \
    --input wav/brian_g8sez.wav \
    --output output_mpp.wav \
    --EbNodB 6 \
    --multipath-h-file h_mpp.f32

# Multipath with specific Eb/No
python3 inference.py \
    --model-path model19_check3/checkpoints/checkpoint_epoch_100.pth \
    --input wav/brian_g8sez.wav \
    --output output_mpp_3dB.wav \
    --EbNodB 3 \
    --multipath-h-file h_mpp.f32
```

**Generate custom multipath channel:**

```octave
# In Octave/MATLAB
octave:1> multipath_samples("custom", 8000, 2000, 1, 10, "h_custom.f32")
# Creates h_custom.f32 with custom multipath characteristics

# Then use in inference:
python3 inference.py \
    --model-path model19_check3/checkpoints/checkpoint_epoch_100.pth \
    --input wav/brian_g8sez.wav \
    --output output_custom_mp.wav \
    --EbNodB 6 \
    --multipath-h-file h_custom.f32
```

### 4. Full TX/RX Pipeline with IQ Samples

**Step 1: Transmit (create IQ samples)**

```bash
# Generate IQ samples for transmission
python3 radae_txe.py \
    --model-path model19_check3/checkpoints/checkpoint_epoch_100.pth \
    --input wav/brian_g8sez.wav \
    --output tx_iq.f32 \
    --pilots
```

**What was created:**
- `tx_iq.f32` - Complex IQ samples at 8 kHz sample rate
- Format: 32-bit float, interleaved I/Q (real, imag, real, imag, ...)
- Ready for transmission via SDR or file transfer

**Step 2: Simulate channel (optional)**

```bash
# Add AWGN noise to IQ samples
python3 -c "
import numpy as np

# Load IQ samples
iq = np.fromfile('tx_iq.f32', dtype=np.float32)
iq_complex = iq[0::2] + 1j * iq[1::2]

# Add noise (SNR = 10 dB)
signal_power = np.mean(np.abs(iq_complex)**2)
noise_power = signal_power / (10**(10/10))
noise = np.sqrt(noise_power/2) * (np.random.randn(len(iq_complex)) +
                                    1j * np.random.randn(len(iq_complex)))
iq_noisy = iq_complex + noise

# Save
iq_out = np.zeros(len(iq), dtype=np.float32)
iq_out[0::2] = iq_noisy.real
iq_out[1::2] = iq_noisy.imag
iq_out.tofile('rx_iq.f32')
print(f'Created rx_iq.f32 with SNR=10dB')
"
```

**Step 3: Receive (decode IQ samples)**

```bash
# Decode IQ samples
python3 radae_rxe.py \
    --model-path model19_check3/checkpoints/checkpoint_epoch_100.pth \
    --input rx_iq.f32 \
    --output output_decoded.wav \
    --pilots
```

**Step 4: Verify quality**

```bash
# Compare with original
echo "Original:"
soxi wav/brian_g8sez.wav
echo ""
echo "Received:"
soxi output_decoded.wav

# Calculate loss
python3 loss.py features_in.f32 features_out.f32
# Expected loss: <0.15 for good quality
```

### 5. Standalone Receiver (rx.py)

**For real-time or file-based reception:**

```bash
# Generate TX IQ with inference.py first
python3 inference.py \
    --model-path model19_check3/checkpoints/checkpoint_epoch_100.pth \
    --input wav/brian_g8sez.wav \
    --write-latent z.f32 \
    --write-rx rx_in.f32

# Receive with standalone rx.py
python3 rx.py \
    --model-path model19_check3/checkpoints/checkpoint_epoch_100.pth \
    --input rx_in.f32 \
    --output rx_out.wav
```

---

## Model Variants

### Model Comparison Table

| Model | Bottleneck | Latent Dim | Auxdata | PAPR | Best For |
|-------|------------|------------|---------|------|----------|
| **model19_check3** | 3 (PAPR opt) | 80 | Yes (25 bits/s) | <1dB | **Production (recommended)** |
| model05 | 1 (basic) | 80 | No | High | Reference/baseline |
| model17 | 3 (PAPR opt) | 80 | No | <1dB | Good alternative |
| model18 | 3 (PAPR opt) | 40 | No | <1dB | Lower bandwidth |

### Using Different Models

**Model05 (Reference baseline):**
```bash
./inference.sh model05/checkpoints/checkpoint_epoch_100.pth \
    wav/brian_g8sez.wav \
    output_model05.wav \
    --EbNodB 6
```

**Model17 (PAPR optimized):**
```bash
./inference.sh model17/checkpoints/checkpoint_epoch_100.pth \
    wav/brian_g8sez.wav \
    output_model17.wav \
    --EbNodB 6
```

**Model18 (Lower bandwidth - 40 dims):**
```bash
./inference.sh model18/checkpoints/checkpoint_epoch_100.pth \
    wav/brian_g8sez.wav \
    output_model18.wav \
    --EbNodB 6 \
    --latent-dim 40
```

**Model19_check3 (Production - with auxdata):**
```bash
./inference.sh model19_check3/checkpoints/checkpoint_epoch_100.pth \
    wav/brian_g8sez.wav \
    output_model19.wav \
    --EbNodB 6 \
    --auxdata
```

### Compare Models Side-by-Side

```bash
# Compare all models at same SNR
./compare_models.sh 6

# This generates:
# - Loss vs Eb/No curves
# - Audio samples for each model
# - Performance comparison plots
```

---

## BBFM (Baseband FM)

### Overview

BBFM is designed for **VHF/UHF land mobile radio** (not HF):
- Single carrier PSK (not OFDM)
- FM modulation (not SSB)
- Different channel model (LMR fading)
- DC-coupled and passband FM radios

### Basic BBFM Inference

**1. Perfect channel:**
```bash
./inference_bbfm.sh model_bbfm_01/checkpoints/checkpoint_epoch_100.pth \
    wav/brian_g8sez.wav \
    output_bbfm.wav
```

**2. With channel simulation:**
```bash
python3 inference_bbfm.py \
    --model-path model_bbfm_01/checkpoints/checkpoint_epoch_100.pth \
    --input wav/brian_g8sez.wav \
    --output output_bbfm_noisy.wav \
    --EbNodB 10 \
    --write-latent z_bbfm.f32
```

### BBFM with Single Carrier Modem

**Step 1: Generate latent symbols**
```bash
./inference_bbfm.sh model_bbfm_01/checkpoints/checkpoint_epoch_100.pth \
    wav/brian_g8sez.wav \
    - \
    --write-latent z_bbfm.f32
```

**Step 2: Modulate with single carrier modem**
```bash
# Transmit (creates int16 samples at 9.6 kHz for FM radio)
cat z_bbfm.f32 | python3 sc_tx.py > tx_fm.int16

# Can play this into FM radio input
# sox -t .s16 -r 9600 -c 1 tx_fm.int16 -t alsa default
```

**Step 3: Receive and demodulate**
```bash
# Receive from FM radio output (or file)
cat tx_fm.int16 | python3 sc_rx.py > z_bbfm_rx.f32
```

**Step 4: Decode to audio**
```bash
./rx_bbfm.sh model_bbfm_01/checkpoints/checkpoint_epoch_100.pth \
    z_bbfm_rx.f32 \
    output_bbfm_decoded.wav
```

### BBFM with LMR Channel Simulation

**Generate LMR fading channel:**
```octave
# In Octave
octave:1> multipath_samples("lmr60", 8000, 2000, 1, 10, "h_lmr60.f32")
# Creates h_lmr60.f32 for 60 km/hr @ 450 MHz
```

**Run inference with LMR channel:**
```bash
python3 inference_bbfm.py \
    --model-path model_bbfm_01/checkpoints/checkpoint_epoch_100.pth \
    --input wav/brian_g8sez.wav \
    --output output_bbfm_lmr.wav \
    --EbNodB 10 \
    --h-file h_lmr60.f32
```

### Analog FM Comparison

**Compare BBFM vs analog FM:**
```bash
./analog_bbfm.sh wav/brian_g8sez.wav 10

# This runs:
# 1. Analog FM simulation
# 2. BBFM simulation
# 3. Comparison plots and audio outputs
```

---

## RADAE v2 (Experimental)

### Overview

RADEv2 is on the experimental `dr-radev2` branch with:
- ML-based synchronization
- ML-based equalization
- Fine-tuning support
- Adaptive channel estimation

### Checkout RADEv2

```bash
cd ~/radae
git fetch --all
git checkout dr-radev2

# Build (same as main)
cd build
make -j$(nproc)
cd ..
```

### Using ML Synchronization

**1. Train ML sync model (if needed):**
```bash
python3 train_ml_sync.py \
    --output-dir ml_sync_model \
    --epochs 50
```

**2. Run inference with ML sync:**
```bash
python3 inference.py \
    --model-path model19_check3/checkpoints/checkpoint_epoch_100.pth \
    --input wav/brian_g8sez.wav \
    --output output_v2_ml_sync.wav \
    --ml-sync \
    --ml-sync-model ml_sync_model/checkpoint.pth \
    --EbNodB 3
```

### Using ML Equalization

```bash
python3 ml_eq.py \
    --model-path model19_check3/checkpoints/checkpoint_epoch_100.pth \
    --input wav/brian_g8sez.wav \
    --output output_v2_ml_eq.wav \
    --ml-eq-model mleq_models/checkpoint.pth \
    --EbNodB 3 \
    --multipath-h-file h_mpp.f32
```

### Fine-Tuning for Specific Channels

```bash
# Fine-tune model for specific channel conditions
python3 train.py \
    --fine-tune model19_check3/checkpoints/checkpoint_epoch_100.pth \
    --h-file h_custom_channel.f32 \
    --epochs 10 \
    --output fine_tuned_model

# Use fine-tuned model
./inference.sh fine_tuned_model/checkpoint_epoch_10.pth \
    wav/brian_g8sez.wav \
    output_fine_tuned.wav \
    --EbNodB 3
```

---

## Audio Format Conversion

### Convert MP3 to WAV

**Using SoX:**
```bash
# Convert MP3 to 16kHz mono WAV (required format)
sox input.mp3 -r 16000 -c 1 output.wav

# Or with explicit format
sox input.mp3 -t wav -r 16000 -c 1 -b 16 output.wav
```

**Using FFmpeg:**
```bash
# Convert MP3 to 16kHz mono WAV
ffmpeg -i input.mp3 -ar 16000 -ac 1 output.wav

# Convert any format to RADAE-compatible WAV
ffmpeg -i input.{mp4,m4a,flac,ogg} -ar 16000 -ac 1 output.wav
```

### Batch Conversion

```bash
# Convert all MP3 files in directory
for f in *.mp3; do
    sox "$f" -r 16000 -c 1 "${f%.mp3}.wav"
done

# Or with FFmpeg
for f in *.mp3; do
    ffmpeg -i "$f" -ar 16000 -ac 1 "${f%.mp3}.wav"
done
```

### Prepare Custom Audio

```bash
# Trim to specific duration (10 seconds)
sox input.wav -r 16000 -c 1 output_10s.wav trim 0 10

# Normalize volume
sox input.wav -r 16000 -c 1 output_normalized.wav norm -3

# Remove silence from beginning and end
sox input.wav -r 16000 -c 1 output_trimmed.wav silence 1 0.1 1% reverse silence 1 0.1 1% reverse

# Chain operations
sox input.mp3 -r 16000 -c 1 output.wav \
    silence 1 0.1 1% reverse silence 1 0.1 1% reverse \
    norm -3
```

---

## Over-the-Air Testing

### Automated OTA Test

```bash
# Run automated over-the-air test
./ota_test.sh model19_check3/checkpoints/checkpoint_epoch_100.pth

# This will:
# 1. Transmit test audio via SDR
# 2. Receive via SDR
# 3. Decode and compare
# 4. Generate quality metrics
```

### Manual OTA with SDR

**Transmit with HackRF/PlutoSDR/USRP:**

```bash
# Generate IQ samples
python3 radae_txe.py \
    --model-path model19_check3/checkpoints/checkpoint_epoch_100.pth \
    --input wav/brian_g8sez.wav \
    --output tx_iq.f32

# Convert to complex int16 for SDR
python3 f32toint16.py --scale 8192 < tx_iq.f32 > tx_iq.int16

# Transmit with HackRF
hackrf_transfer -t tx_iq.int16 -f 14100000 -s 8000000 -x 30

# Or with PlutoSDR (via IIO)
iio_attr -c ad9361-phy TX_LO frequency 14100000
cat tx_iq.int16 > /dev/iio\:device2
```

**Receive with SDR:**

```bash
# Receive with HackRF
hackrf_transfer -r rx_iq.int16 -f 14100000 -s 8000000 -l 32 -g 40

# Convert to float32
python3 int16tof32.py < rx_iq.int16 > rx_iq.f32

# Decode
python3 radae_rxe.py \
    --model-path model19_check3/checkpoints/checkpoint_epoch_100.pth \
    --input rx_iq.f32 \
    --output rx_audio.wav
```

---

## Performance Comparison

### Compare All Variants

```bash
# Test audio file
INPUT="wav/brian_g8sez.wav"
SNR=6

# HF RADAE - model19_check3
./inference.sh model19_check3/checkpoints/checkpoint_epoch_100.pth \
    $INPUT out_hf_m19.wav --EbNodB $SNR

# HF RADAE - model05 (baseline)
./inference.sh model05/checkpoints/checkpoint_epoch_100.pth \
    $INPUT out_hf_m05.wav --EbNodB $SNR

# HF RADAE - model18 (low bandwidth)
./inference.sh model18/checkpoints/checkpoint_epoch_100.pth \
    $INPUT out_hf_m18.wav --EbNodB $SNR --latent-dim 40

# BBFM
./inference_bbfm.sh model_bbfm_01/checkpoints/checkpoint_epoch_100.pth \
    $INPUT out_bbfm.wav --EbNodB $SNR

# Play all outputs
for f in out_*.wav; do
    echo "Playing: $f"
    aplay "$f"
done
```

### Quality Metrics

```bash
# Calculate loss (MSE of features)
python3 loss.py features_in.f32 features_out_hf_m19.f32
python3 loss.py features_in.f32 features_out_hf_m05.f32
python3 loss.py features_in.f32 features_out_bbfm.f32

# Expected results:
# model19_check3: ~0.10-0.12 (excellent)
# model05: ~0.14-0.16 (good)
# BBFM: ~0.12-0.15 (good for VHF)
```

### Evaluation Script

```bash
# Comprehensive evaluation vs SSB
./evaluate.sh model19_check3/checkpoints/checkpoint_epoch_100.pth \
    wav/brian_g8sez.wav \
    6

# Generates:
# - Output audio samples
# - Spectrograms
# - Quality comparison plots
# - Loss metrics
```

---

## Troubleshooting

### Common Issues

**1. "ModuleNotFoundError: No module named 'radae'"**
```bash
# Solution: Add radae to Python path
export PYTHONPATH=$HOME/radae:$PYTHONPATH

# Or install as package
cd ~/radae
pip3 install -e .
```

**2. "RuntimeError: CUDA not available"**
```bash
# Solution: Use CPU mode (default)
python3 inference.py ... --no-cuda

# Or install CUDA PyTorch (if you have GPU)
pip3 install torch torchvision torchaudio
```

**3. "File not found: checkpoint_epoch_100.pth"**
```bash
# Solution: Ensure model is downloaded
cd ~/radae
ls -lh model19_check3/checkpoints/

# If missing, clone again or download specific model
wget https://github.com/drowe67/radae/releases/download/v0.1/model19_check3.tar.gz
tar xzf model19_check3.tar.gz
```

**4. Audio quality is poor**
```bash
# Check input audio format
soxi input.wav
# Should be: 16kHz, 1 channel, 16-bit PCM

# Convert if needed
sox input.wav -r 16000 -c 1 input_fixed.wav

# Try higher SNR
./inference.sh model19_check3/checkpoints/checkpoint_epoch_100.pth \
    input_fixed.wav output.wav --EbNodB 10
```

**5. "lpcnet_demo not found"**
```bash
# Solution: Build C components
cd ~/radae/build
cmake ..
make -j$(nproc)

# Add to PATH
export PATH=$HOME/radae/build/src:$PATH
```

**6. Sync issues with rx.py**
```bash
# Solution: Enable pilots and increase acquisition range
python3 rx.py \
    --model-path model19_check3/checkpoints/checkpoint_epoch_100.pth \
    --input rx_iq.f32 \
    --output output.wav \
    --pilots \
    --acquisition-range 100
```

---

## Quick Reference Commands

### HF RADAE (Recommended - model19_check3)

```bash
# Perfect channel
./inference.sh model19_check3/checkpoints/checkpoint_epoch_100.pth input.wav output.wav --EbNodB 100

# AWGN (6 dB SNR)
./inference.sh model19_check3/checkpoints/checkpoint_epoch_100.pth input.wav output.wav --EbNodB 6

# Multipath
python3 inference.py --model-path model19_check3/checkpoints/checkpoint_epoch_100.pth \
    --input input.wav --output output.wav --EbNodB 6 --multipath-h-file h_mpp.f32

# Full TX/RX pipeline
python3 radae_txe.py --model-path model19_check3/checkpoints/checkpoint_epoch_100.pth \
    --input input.wav --output tx.f32 --pilots
python3 radae_rxe.py --model-path model19_check3/checkpoints/checkpoint_epoch_100.pth \
    --input tx.f32 --output output.wav --pilots
```

### BBFM (VHF/UHF)

```bash
# Basic
./inference_bbfm.sh model_bbfm_01/checkpoints/checkpoint_epoch_100.pth input.wav output.wav

# With modem
./inference_bbfm.sh model_bbfm_01/checkpoints/checkpoint_epoch_100.pth input.wav - --write-latent z.f32
cat z.f32 | python3 sc_tx.py > tx.int16
cat tx.int16 | python3 sc_rx.py > z_rx.f32
./rx_bbfm.sh model_bbfm_01/checkpoints/checkpoint_epoch_100.pth z_rx.f32 output.wav
```

### Audio Conversion

```bash
# MP3 to WAV (16kHz mono)
sox input.mp3 -r 16000 -c 1 output.wav

# Batch convert
for f in *.mp3; do sox "$f" -r 16000 -c 1 "${f%.mp3}.wav"; done
```

---

## Appendix: File Formats

### IQ Sample Files (.f32)

**Format:**
- 32-bit float
- Interleaved I/Q: [I, Q, I, Q, I, Q, ...]
- Sample rate: 8000 Hz (for HF RADAE)
- Complex samples: I + jQ

**Read/Write Example:**
```python
import numpy as np

# Read
iq = np.fromfile('samples.f32', dtype=np.float32)
iq_complex = iq[0::2] + 1j * iq[1::2]

# Write
iq_out = np.zeros(len(iq_complex)*2, dtype=np.float32)
iq_out[0::2] = iq_complex.real
iq_out[1::2] = iq_complex.imag
iq_out.tofile('samples_out.f32')
```

### Feature Files (.f32)

**Format:**
- 32-bit float
- FARGAN features: 20 or 21 features per 10ms frame
- Sequential: [f1, f2, ..., f20, f1, f2, ..., f20, ...]

### Latent Symbol Files (.f32)

**Format:**
- 32-bit float
- 80 dimensions (or 40 for model18)
- One vector per 40ms (25 Hz rate)

---

**Document Version:** 1.0
**Last Updated:** 2025-11-17
**RADAE Version:** main branch (production)
