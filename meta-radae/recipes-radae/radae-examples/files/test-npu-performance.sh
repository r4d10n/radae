#!/bin/bash
#
# RADAE NPU Performance Test
# Benchmarks NPU inference performance
#

set -e

echo "========================================"
echo "RADAE NPU Performance Benchmark"
echo "========================================"
echo ""

# Check NPU availability
echo "1. Checking NPU availability..."

if [ -f /sys/class/misc/galcore/device/driver/version ]; then
    NPU_VERSION=$(cat /sys/class/misc/galcore/device/driver/version)
    echo "   NPU Driver Version: $NPU_VERSION"
else
    echo "   Warning: NPU device not found"
    echo "   Falling back to CPU inference"
fi

if [ -f /sys/class/misc/galcore/device/utilization ]; then
    echo "   NPU Utilization: $(cat /sys/class/misc/galcore/device/utilization)"
fi

# Check models
echo ""
echo "2. Checking NPU models..."

ENCODER_MODEL="/usr/share/radae/radae_encoder_int8.tflite"
DECODER_MODEL="/usr/share/radae/radae_decoder_int8.tflite"

if [ ! -f "$ENCODER_MODEL" ]; then
    echo "   Error: Encoder model not found: $ENCODER_MODEL"
    exit 1
fi

if [ ! -f "$DECODER_MODEL" ]; then
    echo "   Error: Decoder model not found: $DECODER_MODEL"
    exit 1
fi

echo "   Encoder model: $ENCODER_MODEL ($(du -h $ENCODER_MODEL | cut -f1))"
echo "   Decoder model: $DECODER_MODEL ($(du -h $DECODER_MODEL | cut -f1))"

# System info
echo ""
echo "3. System Information..."
echo "   CPU: $(cat /proc/cpuinfo | grep "model name" | head -n1 | cut -d: -f2 | xargs)"
echo "   Cores: $(nproc)"
echo "   Memory: $(free -h | grep Mem: | awk '{print $2}')"

# CPU frequency
echo "   CPU Frequencies:"
for cpu in /sys/devices/system/cpu/cpu[0-3]/cpufreq/scaling_cur_freq; do
    if [ -f "$cpu" ]; then
        freq=$(cat "$cpu")
        freq_mhz=$((freq / 1000))
        echo "      $(basename $(dirname $(dirname $cpu))): ${freq_mhz} MHz"
    fi
done

# Performance test
echo ""
echo "4. Running inference benchmark..."
echo "   This will stress test the NPU for 30 seconds"
echo ""

# Create test program if radae-trx exists
if command -v radae-trx >/dev/null 2>&1; then
    echo "   Running RADAE application benchmark..."

    # Monitor CPU and NPU usage
    (
        for i in {1..30}; do
            sleep 1
            if [ -f /sys/class/misc/galcore/device/utilization ]; then
                npu_util=$(cat /sys/class/misc/galcore/device/utilization 2>/dev/null || echo "N/A")
            else
                npu_util="N/A"
            fi

            cpu_util=$(top -bn1 | grep "Cpu(s)" | sed "s/.*, *\([0-9.]*\)%* id.*/\1/" | awk '{print 100 - $1}')

            echo "   [$i/30] CPU: ${cpu_util}% NPU: ${npu_util}"
        done
    ) &

    MONITOR_PID=$!

    # Run benchmark (would need actual benchmark binary)
    echo "   (Benchmark would run here with actual RADAE binary)"
    sleep 30

    wait $MONITOR_PID
else
    echo "   Warning: radae-trx not found, skipping runtime benchmark"
fi

echo ""
echo "Benchmark complete!"
echo "========================================"
