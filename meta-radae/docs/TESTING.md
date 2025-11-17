# RADAE Testing Guide

**Version:** 1.0
**Platform:** NXP i.MX 8M Plus
**Date:** 2025-11-17

## Table of Contents

1. [Testing Overview](#testing-overview)
2. [Unit Tests](#unit-tests)
3. [Integration Tests](#integration-tests)
4. [Performance Benchmarks](#performance-benchmarks)
5. [NPU Verification](#npu-verification)
6. [Audio Loopback Tests](#audio-loopback-tests)
7. [Over-the-Air Tests](#over-the-air-tests)
8. [Automated Testing](#automated-testing)

---

## Testing Overview

This guide provides comprehensive testing procedures for RADAE on i.MX 8M Plus, covering:
- Unit tests for individual components
- Integration tests for full system
- Performance benchmarks (CPU, NPU, latency)
- Audio quality verification
- Over-the-air radio tests

### Test Environment

**Required Hardware:**
- i.MX 8M Plus EVK or compatible board
- Serial console cable (115200 8N1)
- Network connection (Ethernet or WiFi)
- USB audio adapter or I2S codec
- HF radio transceiver (for OTA tests)

**Required Software:**
- Built RADAE image flashed to SD card
- Host PC with SSH client
- Audio analysis tools (optional)

---

## Unit Tests

### NPU Driver Test

Verify NPU driver is loaded and functioning:

```bash
# On target device

# Test 1: Check NPU device node
test -c /dev/galcore && echo "NPU device: PASS" || echo "NPU device: FAIL"

# Test 2: Check NPU kernel module
lsmod | grep -q galcore && echo "NPU module: PASS" || echo "NPU module: FAIL"

# Test 3: Check NPU driver version
if [ -f /sys/class/misc/galcore/device/driver/version ]; then
    echo "NPU version: PASS"
    cat /sys/class/misc/galcore/device/driver/version
else
    echo "NPU version: FAIL"
fi

# Test 4: NPU memory allocation
dmesg | grep -q "galcore.*reserved memory" && echo "NPU memory: PASS" || echo "NPU memory: WARN"
```

**Expected Output:**
```
NPU device: PASS
NPU module: PASS
NPU version: PASS
GC kernel version: 6.4.11.xxxxx
NPU memory: PASS
```

### TensorFlow Lite Test

Verify TensorFlow Lite installation:

```bash
# Test Python TFLite runtime
python3 << 'EOF'
import sys

try:
    import tflite_runtime.interpreter as tflite
    print(f"TFLite runtime: PASS (version {tflite.__version__})")
except ImportError as e:
    print(f"TFLite runtime: FAIL ({e})")
    sys.exit(1)

# Test NPU delegate loading
try:
    delegate = tflite.load_delegate('/usr/lib/libvx_delegate.so')
    print("NPU delegate: PASS")
except Exception as e:
    print(f"NPU delegate: FAIL ({e})")
    sys.exit(1)

# Test basic inference
import numpy as np

# Create simple model for testing
interpreter = tflite.Interpreter(
    model_path='/usr/share/radae/models/radae_encoder_int8.tflite',
    experimental_delegates=[delegate]
)
interpreter.allocate_tensors()

input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()

print(f"Model loaded: PASS")
print(f"  Input shape: {input_details[0]['shape']}")
print(f"  Output shape: {output_details[0]['shape']}")

# Test inference
test_input = np.random.randn(*input_details[0]['shape']).astype(np.float32)

# Quantize if needed
if input_details[0]['dtype'] == np.int8:
    scale = input_details[0]['quantization'][0]
    zero_point = input_details[0]['quantization'][1]
    test_input = (test_input / scale + zero_point).astype(np.int8)

interpreter.set_tensor(input_details[0]['index'], test_input)
interpreter.invoke()
output = interpreter.get_tensor(output_details[0]['index'])

print(f"Inference: PASS (output shape: {output.shape})")
EOF
```

**Expected Output:**
```
TFLite runtime: PASS (version 2.12.x)
NPU delegate: PASS
Model loaded: PASS
  Input shape: [1 4 20]
  Output shape: [1 80]
Inference: PASS (output shape: (1, 80))
```

### RADAE Encoder/Decoder Test

Test individual RADAE components:

```bash
# Test encoder
radae-npu-test --mode encoder \
    --input /usr/share/radae/test/audio_sample.wav \
    --output /tmp/latent.bin

# Check output
if [ -f /tmp/latent.bin ] && [ -s /tmp/latent.bin ]; then
    echo "Encoder: PASS"
    ls -lh /tmp/latent.bin
else
    echo "Encoder: FAIL"
fi

# Test decoder
radae-npu-test --mode decoder \
    --input /tmp/latent.bin \
    --output /tmp/decoded_features.bin

# Check output
if [ -f /tmp/decoded_features.bin ] && [ -s /tmp/decoded_features.bin ]; then
    echo "Decoder: PASS"
    ls -lh /tmp/decoded_features.bin
else
    echo "Decoder: FAIL"
fi
```

### Audio I/O Test

Test audio capture and playback:

```bash
# Test 1: List audio devices
echo "=== Audio Devices ==="
aplay -l
arecord -l

# Test 2: Playback test
echo "=== Playback Test ==="
speaker-test -c 1 -t wav -l 1 && echo "Playback: PASS" || echo "Playback: FAIL"

# Test 3: Capture test (5 seconds)
echo "=== Capture Test ==="
arecord -D hw:0,0 -f S16_LE -r 16000 -c 1 -d 5 /tmp/capture_test.wav
if [ -f /tmp/capture_test.wav ] && [ -s /tmp/capture_test.wav ]; then
    echo "Capture: PASS"
    ls -lh /tmp/capture_test.wav
else
    echo "Capture: FAIL"
fi

# Test 4: Loopback (capture and immediate playback)
echo "=== Loopback Test (speaking for 3 seconds) ==="
arecord -D hw:0,0 -f S16_LE -r 16000 -c 1 -d 3 /tmp/loopback.wav
aplay -D hw:0,0 /tmp/loopback.wav
echo "Loopback: MANUAL (verify audio quality)"
```

---

## Integration Tests

### End-to-End Pipeline Test

Test complete RADAE TX → RX pipeline:

```bash
# Test with sample audio file
AUDIO_INPUT="/usr/share/radae/test/test_voice.wav"
AUDIO_OUTPUT="/tmp/radae_output.wav"

# Run full pipeline
radae-npu-test --mode pipeline \
    --input $AUDIO_INPUT \
    --output $AUDIO_OUTPUT \
    --benchmark

# Verify output
if [ -f $AUDIO_OUTPUT ]; then
    echo "Pipeline: PASS"
    echo "Input size:  $(stat -c%s $AUDIO_INPUT) bytes"
    echo "Output size: $(stat -c%s $AUDIO_OUTPUT) bytes"
    
    # Compare durations (should be similar)
    soxi -D $AUDIO_INPUT
    soxi -D $AUDIO_OUTPUT
else
    echo "Pipeline: FAIL"
fi
```

### OFDM Modem Test

Test OFDM modulation/demodulation:

```bash
# Generate test symbols
radae-ofdm-test --mode modulate \
    --input /tmp/latent.bin \
    --output /tmp/ofdm_symbols.iq \
    --Fs 8000 --Nc 30 --Rs 50

# Demodulate
radae-ofdm-test --mode demodulate \
    --input /tmp/ofdm_symbols.iq \
    --output /tmp/latent_recovered.bin \
    --Fs 8000 --Nc 30 --Rs 50

# Compare original and recovered
python3 << 'EOF'
import numpy as np

original = np.fromfile('/tmp/latent.bin', dtype=np.float32)
recovered = np.fromfile('/tmp/latent_recovered.bin', dtype=np.float32)

mse = np.mean((original - recovered)**2)
print(f"OFDM MSE: {mse:.6f}")

if mse < 0.01:
    print("OFDM: PASS (MSE < 0.01)")
else:
    print("OFDM: FAIL (MSE too high)")
EOF
```

### Streaming Mode Test

Test real-time streaming:

```bash
# Start receiver in background
radae-rx --input hw:0,0 --output hw:1,0 --mode npu &
RX_PID=$!
sleep 2

# Start transmitter
timeout 10 radae-tx --input hw:0,0 --output hw:1,0 --mode npu &
TX_PID=$!

# Wait for completion
wait $TX_PID
kill $RX_PID

echo "Streaming: MANUAL (verify audio quality)"
```

---

## Performance Benchmarks

### CPU/NPU Performance Benchmark

```bash
# Run comprehensive benchmark
radae-npu-test --benchmark --iterations 100

# Expected output:
# === RADAE Performance Benchmark ===
# Hardware: i.MX 8M Plus (NPU enabled)
# Iterations: 100
# 
# Encoder (NPU):
#   Mean: 3.2 ms
#   Min:  2.8 ms
#   Max:  4.1 ms
#   Real-time factor: 12.5× (40ms / 3.2ms)
# 
# Decoder (NPU):
#   Mean: 3.5 ms
#   Min:  3.1 ms
#   Max:  4.3 ms
#   Real-time factor: 11.4× (40ms / 3.5ms)
# 
# FARGAN Vocoder (CPU):
#   Mean: 22.1 ms
#   Min:  20.3 ms
#   Max:  25.7 ms
#   Real-time factor: 0.45× (10ms / 22.1ms)
# 
# Total Pipeline (per 10ms frame):
#   Mean: 8.2 ms
#   Real-time factor: 1.22× (10ms / 8.2ms)
```

### CPU vs NPU Comparison

```bash
# Benchmark with NPU
echo "=== NPU Mode ==="
radae-npu-test --mode encoder --backend npu --benchmark --iterations 50

# Benchmark with CPU fallback
echo "=== CPU Mode ==="
radae-npu-test --mode encoder --backend cpu --benchmark --iterations 50

# Compare results
python3 << 'EOF'
# Parse results and compute speedup
# (Script would read benchmark logs and compute speedup factor)
# Expected: NPU is 8-10× faster than CPU for encoder/decoder
EOF
```

### Memory Usage Benchmark

```bash
# Monitor memory usage during operation
echo "=== Memory Benchmark ==="
free -m

# Start RADAE process
radae-npu-test --mode pipeline \
    --input /usr/share/radae/test/long_audio.wav \
    --output /dev/null &
RADAE_PID=$!

# Monitor memory every second for 10 seconds
for i in {1..10}; do
    echo "T+$i seconds:"
    ps -p $RADAE_PID -o pid,vsz,rss,pmem,comm
    sleep 1
done

kill $RADAE_PID
wait

# Expected:
# VSZ: 150-250 MB (virtual memory)
# RSS:  50-100 MB (resident memory)
# %MEM: 2-5%
```

### Latency Measurement

```bash
# Measure end-to-end latency
radae-npu-test --mode latency

# Expected output:
# === Latency Breakdown ===
# Feature extraction:     10 ms
# Encoder buffering:      40 ms
# Encoder processing:      3 ms
# OFDM frame:            120 ms
# OFDM demod:              1 ms
# Decoder processing:      3 ms
# Vocoder synthesis:      22 ms
# ---------------------------
# Total:                 199 ms
```

### Power Consumption Test

```bash
# Measure power consumption (if available)
# Note: Requires INA226 power monitor or similar

# Idle power
echo "Measuring idle power..."
sleep 5
# Read power sensor
cat /sys/class/hwmon/hwmon0/power1_input  # μW

# Active RADAE (NPU mode)
echo "Measuring active power (NPU)..."
radae-npu-test --mode pipeline --input /usr/share/radae/test/audio.wav --output /dev/null &
RADAE_PID=$!
sleep 5
# Read power sensor
cat /sys/class/hwmon/hwmon0/power1_input  # μW
kill $RADAE_PID

# Active RADAE (CPU mode)
echo "Measuring active power (CPU)..."
radae-npu-test --mode pipeline --backend cpu --input /usr/share/radae/test/audio.wav --output /dev/null &
RADAE_PID=$!
sleep 5
# Read power sensor
cat /sys/class/hwmon/hwmon0/power1_input  # μW
kill $RADAE_PID

# Expected:
# Idle: 2-3 W
# NPU mode: 5-6 W (RADAE adds ~3W)
# CPU mode: 8-10 W (RADAE adds ~6W)
# Power savings: 40-50% with NPU
```

---

## NPU Verification

### NPU Utilization Test

```bash
# Check NPU utilization during inference
# (Requires custom NPU monitoring, if available)

# Start RADAE in background
radae-npu-test --mode pipeline --input /usr/share/radae/test/audio.wav --output /dev/null &
RADAE_PID=$!

# Monitor NPU (if sysfs interface available)
for i in {1..10}; do
    if [ -f /sys/class/misc/galcore/device/utilization ]; then
        echo "T+$i: NPU utilization: $(cat /sys/class/misc/galcore/device/utilization)%"
    fi
    sleep 1
done

kill $RADAE_PID

# Expected NPU utilization: 40-60% during active inference
```

### NPU Frequency Scaling

```bash
# Check NPU frequency governor
if [ -f /sys/class/misc/galcore/device/governor ]; then
    echo "Current NPU governor: $(cat /sys/class/misc/galcore/device/governor)"
    
    # Test performance governor
    echo performance > /sys/class/misc/galcore/device/governor
    radae-npu-test --mode encoder --benchmark --iterations 50
    
    # Test powersave governor
    echo powersave > /sys/class/misc/galcore/device/governor
    radae-npu-test --mode encoder --benchmark --iterations 50
    
    # Compare performance
fi
```

### NPU Model Validation

```bash
# Validate NPU model accuracy vs CPU reference
python3 << 'EOF'
import numpy as np
import tflite_runtime.interpreter as tflite

# Load model with NPU delegate
npu_delegate = tflite.load_delegate('/usr/lib/libvx_delegate.so')
interpreter_npu = tflite.Interpreter(
    model_path='/usr/share/radae/models/radae_encoder_int8.tflite',
    experimental_delegates=[npu_delegate]
)
interpreter_npu.allocate_tensors()

# Load model for CPU
interpreter_cpu = tflite.Interpreter(
    model_path='/usr/share/radae/models/radae_encoder_int8.tflite'
)
interpreter_cpu.allocate_tensors()

# Test with random inputs
input_details = interpreter_npu.get_input_details()
output_details = interpreter_npu.get_output_details()

test_input = np.random.randn(1, 4, 20).astype(np.float32)

# NPU inference
if input_details[0]['dtype'] == np.int8:
    scale = input_details[0]['quantization'][0]
    zero_point = input_details[0]['quantization'][1]
    test_input_quant = (test_input / scale + zero_point).astype(np.int8)
else:
    test_input_quant = test_input

interpreter_npu.set_tensor(input_details[0]['index'], test_input_quant)
interpreter_npu.invoke()
output_npu_quant = interpreter_npu.get_tensor(output_details[0]['index'])

# Dequantize
output_scale = output_details[0]['quantization'][0]
output_zero_point = output_details[0]['quantization'][1]
output_npu = (output_npu_quant.astype(np.float32) - output_zero_point) * output_scale

# CPU inference
interpreter_cpu.set_tensor(input_details[0]['index'], test_input_quant)
interpreter_cpu.invoke()
output_cpu_quant = interpreter_cpu.get_tensor(output_details[0]['index'])
output_cpu = (output_cpu_quant.astype(np.float32) - output_zero_point) * output_scale

# Compare
mse = np.mean((output_npu - output_cpu)**2)
max_diff = np.max(np.abs(output_npu - output_cpu))

print(f"NPU vs CPU MSE: {mse:.6f}")
print(f"NPU vs CPU Max Diff: {max_diff:.4f}")

if mse < 0.01:
    print("NPU Accuracy: PASS (MSE < 0.01)")
else:
    print(f"NPU Accuracy: WARN (MSE = {mse:.6f})")
EOF
```

---

## Audio Loopback Tests

### Basic Loopback Test

```bash
# Simple audio loopback without RADAE processing
echo "=== Basic Loopback (No Processing) ==="
arecord -D hw:0,0 -f S16_LE -r 16000 -c 1 -d 5 - | aplay -D hw:0,0 -

# You should hear your voice with minimal delay (~50ms)
```

### RADAE Loopback Test

```bash
# Audio loopback through RADAE encoder/decoder
echo "=== RADAE Loopback ==="

# Method 1: File-based
arecord -D hw:0,0 -f S16_LE -r 16000 -c 1 -d 10 /tmp/input.wav
radae-npu-test --mode pipeline --input /tmp/input.wav --output /tmp/output.wav
aplay /tmp/output.wav

# Method 2: Streaming (real-time)
mkfifo /tmp/radae_pipe
arecord -D hw:0,0 -f S16_LE -r 16000 -c 1 - > /tmp/radae_pipe &
RECORD_PID=$!

radae-npu-test --mode pipeline --input /tmp/radae_pipe --output - | aplay -D hw:0,0 - &
RADAE_PID=$!

echo "Running RADAE loopback for 30 seconds..."
sleep 30

kill $RECORD_PID $RADAE_PID
rm /tmp/radae_pipe
```

### Audio Quality Assessment

```bash
# Compare input and output audio quality
python3 << 'EOF'
import numpy as np
import wave

def snr(signal, noise):
    """Calculate Signal-to-Noise Ratio"""
    signal_power = np.mean(signal**2)
    noise_power = np.mean(noise**2)
    return 10 * np.log10(signal_power / noise_power)

# Load original and processed audio
with wave.open('/tmp/input.wav', 'rb') as f:
    original = np.frombuffer(f.readframes(f.getnframes()), dtype=np.int16)

with wave.open('/tmp/output.wav', 'rb') as f:
    processed = np.frombuffer(f.readframes(f.getnframes()), dtype=np.int16)

# Align signals (account for delay)
# Simple cross-correlation to find delay
correlation = np.correlate(processed, original, mode='full')
delay = len(original) - np.argmax(correlation) - 1

# Align
if delay > 0:
    original_aligned = original[delay:]
    processed_aligned = processed[:len(original_aligned)]
else:
    original_aligned = original
    processed_aligned = processed[-delay:]

# Trim to same length
min_len = min(len(original_aligned), len(processed_aligned))
original_aligned = original_aligned[:min_len]
processed_aligned = processed_aligned[:min_len]

# Calculate SNR
noise = original_aligned.astype(np.float32) - processed_aligned.astype(np.float32)
snr_value = snr(original_aligned.astype(np.float32), noise)

print(f"Audio SNR: {snr_value:.2f} dB")

if snr_value > 15:
    print("Audio Quality: EXCELLENT")
elif snr_value > 10:
    print("Audio Quality: GOOD")
elif snr_value > 5:
    print("Audio Quality: FAIR")
else:
    print("Audio Quality: POOR")
EOF
```

---

## Over-the-Air Tests

### RF Loopback Test

```bash
# Connect TX output to RX input via attenuator (20-30 dB)
# This tests the complete radio path

# Terminal 1: Start receiver
radae-rx --mode npu --input hw:0,0 --output hw:1,0 --verbose

# Terminal 2: Start transmitter
radae-tx --mode npu --input hw:2,0 --output hw:0,0 --ptt-gpio 131 --verbose

# Speak into microphone connected to TX
# Listen to speaker connected to RX
# Verify intelligibility and latency
```

### BER (Bit Error Rate) Test

```bash
# Send known test pattern and measure errors
radae-ber-test --mode transmit --pattern pn9 --duration 60 &
TX_PID=$!

radae-ber-test --mode receive --pattern pn9 --duration 60 --output /tmp/ber_results.txt &
RX_PID=$!

wait $TX_PID $RX_PID

# Analyze results
cat /tmp/ber_results.txt
# Expected output:
# Total bits: 48000
# Error bits: 120
# BER: 2.5e-3
# SNR: 12 dB
```

### Channel Simulation Test

```bash
# Test with simulated multipath channel
radae-channel-sim --channel mpp --snr 0 \
    --input /tmp/input.wav \
    --output /tmp/output_mpp.wav

# Test with AWGN channel
radae-channel-sim --channel awgn --snr 10 \
    --input /tmp/input.wav \
    --output /tmp/output_awgn.wav

# Compare quality
# Expected: RADAE should handle MPP channel better than AWGN at same SNR
```

---

## Automated Testing

### Test Script

Create comprehensive test script:

```bash
#!/bin/bash
# /usr/share/radae/test/run_all_tests.sh

echo "==================================="
echo "RADAE Comprehensive Test Suite"
echo "==================================="

PASS=0
FAIL=0

function run_test() {
    echo -n "Testing $1... "
    if $2; then
        echo "PASS"
        ((PASS++))
    else
        echo "FAIL"
        ((FAIL++))
    fi
}

# NPU Tests
run_test "NPU Device" "test -c /dev/galcore"
run_test "NPU Module" "lsmod | grep -q galcore"
run_test "NPU Version" "test -f /sys/class/misc/galcore/device/driver/version"

# TensorFlow Tests
run_test "TFLite Runtime" "python3 -c 'import tflite_runtime.interpreter'"
run_test "NPU Delegate" "test -f /usr/lib/libvx_delegate.so"

# RADAE Tests
run_test "RADAE Binary" "which radae-npu-test"
run_test "RADAE Models" "test -f /usr/share/radae/models/radae_encoder_int8.tflite"

# Audio Tests
run_test "Audio Playback" "speaker-test -c 1 -t wav -l 1 &>/dev/null"
run_test "Audio Capture" "timeout 2 arecord -D hw:0,0 -f S16_LE -r 16000 -c 1 /tmp/test.wav &>/dev/null"

# Performance Tests
echo ""
echo "Running performance benchmarks..."
radae-npu-test --benchmark --iterations 50

echo ""
echo "==================================="
echo "Test Results: $PASS PASS, $FAIL FAIL"
echo "==================================="

if [ $FAIL -eq 0 ]; then
    echo "All tests PASSED!"
    exit 0
else
    echo "Some tests FAILED!"
    exit 1
fi
```

Make executable and run:

```bash
chmod +x /usr/share/radae/test/run_all_tests.sh
/usr/share/radae/test/run_all_tests.sh
```

### Continuous Testing

Set up systemd timer for periodic testing:

```bash
# /etc/systemd/system/radae-test.service
[Unit]
Description=RADAE Periodic Test
After=network.target

[Service]
Type=oneshot
ExecStart=/usr/share/radae/test/run_all_tests.sh
StandardOutput=journal
StandardError=journal
```

```bash
# /etc/systemd/system/radae-test.timer
[Unit]
Description=RADAE Periodic Test Timer

[Timer]
OnBootSec=5min
OnUnitActiveSec=1h
Persistent=true

[Install]
WantedBy=timers.target
```

Enable timer:

```bash
systemctl enable radae-test.timer
systemctl start radae-test.timer

# Check status
systemctl status radae-test.timer
journalctl -u radae-test.service
```

---

## Test Results Logging

### Automated Logging

```bash
# Create test results directory
mkdir -p /var/log/radae-tests

# Run tests with logging
/usr/share/radae/test/run_all_tests.sh | tee /var/log/radae-tests/test_$(date +%Y%m%d_%H%M%S).log

# Analyze test history
grep "Test Results" /var/log/radae-tests/*.log
```

### Performance Tracking

```bash
# Track performance over time
cat > /usr/local/bin/radae-perf-log << 'EOF'
#!/bin/bash
LOGFILE="/var/log/radae-tests/performance.csv"

# Run benchmark
RESULT=$(radae-npu-test --benchmark --iterations 20 2>&1 | grep "Mean:")

# Parse results
ENCODER_MS=$(echo "$RESULT" | grep "Encoder" | awk '{print $2}')
DECODER_MS=$(echo "$RESULT" | grep "Decoder" | awk '{print $2}')

# Log to CSV
echo "$(date +%s),$ENCODER_MS,$DECODER_MS" >> $LOGFILE
EOF

chmod +x /usr/local/bin/radae-perf-log

# Run periodically via cron
echo "0 * * * * /usr/local/bin/radae-perf-log" | crontab -
```

---

## Troubleshooting Test Failures

### NPU Tests Failing

```bash
# Check NPU driver installation
opkg list-installed | grep npu
# or
rpm -qa | grep npu

# Reinstall if missing
opkg install imx-vsi-npu
# or
dnf install imx-vsi-npu

# Check kernel messages
dmesg | grep -i galcore

# Verify device tree
ls /proc/device-tree/npu/status
cat /proc/device-tree/npu/status  # Should be "okay"
```

### Audio Tests Failing

```bash
# Check ALSA configuration
aplay -L
arecord -L

# Check audio codec
amixer -c 0 contents

# Test with simple tone
speaker-test -c 1 -t sine -f 1000

# Check for muted controls
amixer -c 0 sset 'Master' unmute
amixer -c 0 sset 'Speaker' unmute
amixer -c 0 sset 'Headphone' unmute
```

### Performance Below Expected

```bash
# Check CPU governor
cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor

# Set to performance mode
echo performance | tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# Check CPU frequency
cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq

# Check for thermal throttling
cat /sys/class/thermal/thermal_zone0/temp

# Monitor CPU usage
top -d 1
```

---

## Summary

This testing guide covered:
- Unit tests for NPU, TensorFlow Lite, RADAE components
- Integration tests for full pipeline
- Performance benchmarks (latency, throughput, power)
- NPU verification and validation
- Audio loopback and quality tests
- Over-the-air RF tests
- Automated testing framework

All tests should pass on a properly configured i.MX 8M Plus system with meta-radae.

For build instructions, see [BUILD_GUIDE.md](BUILD_GUIDE.md).
For integration details, see [INTEGRATION.md](INTEGRATION.md).
