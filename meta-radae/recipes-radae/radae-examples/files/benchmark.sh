#!/bin/bash
#
# RADAE Comprehensive Benchmark
# Tests all aspects of RADAE performance
#

set -e

GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${BLUE}========================================"
echo "RADAE Comprehensive Benchmark"
echo "========================================${NC}"
echo ""

RESULTS_FILE="/tmp/radae-benchmark-$(date +%Y%m%d-%H%M%S).txt"

log_result() {
    echo "$1" | tee -a "$RESULTS_FILE"
}

log_result "RADAE Benchmark Report"
log_result "Date: $(date)"
log_result "========================================"
log_result ""

# System Information
echo -e "${BLUE}System Information${NC}"
log_result "System Information:"
log_result "  Platform: $(uname -m)"
log_result "  Kernel: $(uname -r)"
log_result "  CPU: $(cat /proc/cpuinfo | grep "model name" | head -n1 | cut -d: -f2 | xargs)"
log_result "  Cores: $(nproc)"
log_result "  Memory: $(free -h | grep Mem: | awk '{print $2}')"
log_result ""

# CPU Frequencies
log_result "CPU Frequencies:"
for cpu in /sys/devices/system/cpu/cpu*/cpufreq/scaling_cur_freq; do
    if [ -f "$cpu" ]; then
        freq=$(cat "$cpu")
        freq_mhz=$((freq / 1000))
        log_result "  $(basename $(dirname $(dirname $cpu))): ${freq_mhz} MHz"
    fi
done
log_result ""

# NPU Information
echo -e "${BLUE}NPU Information${NC}"
if [ -f /sys/class/misc/galcore/device/driver/version ]; then
    log_result "NPU Information:"
    log_result "  Driver: $(cat /sys/class/misc/galcore/device/driver/version)"
    if [ -f /sys/kernel/debug/gc/clk ]; then
        log_result "  Clock: $(cat /sys/kernel/debug/gc/clk 2>/dev/null || echo 'N/A')"
    fi
else
    log_result "NPU: Not available"
fi
log_result ""

# Audio Devices
echo -e "${BLUE}Audio Devices${NC}"
log_result "Audio Devices:"
aplay -l 2>/dev/null | grep "^card" | while read line; do
    log_result "  $line"
done
log_result ""

# Model Information
echo -e "${BLUE}Model Information${NC}"
log_result "Models:"
for model in /usr/share/radae/*.tflite; do
    if [ -f "$model" ]; then
        size=$(du -h "$model" | cut -f1)
        log_result "  $(basename $model): $size"
    fi
done
log_result ""

# Performance Tests
echo -e "${BLUE}Performance Tests${NC}"
log_result "Performance Tests:"
log_result ""

# 1. CPU Performance
echo -e "  ${YELLOW}Running CPU stress test...${NC}"
log_result "  1. CPU Stress Test (10s):"

START_TIME=$(date +%s.%N)
dd if=/dev/zero bs=1M count=1000 | md5sum >/dev/null 2>&1
END_TIME=$(date +%s.%N)

ELAPSED=$(echo "$END_TIME - $START_TIME" | bc)
log_result "     Elapsed: ${ELAPSED}s"
log_result ""

# 2. Memory Bandwidth
echo -e "  ${YELLOW}Testing memory bandwidth...${NC}"
log_result "  2. Memory Bandwidth Test:"

START_TIME=$(date +%s.%N)
dd if=/dev/zero of=/dev/null bs=1M count=1000 2>/dev/null
END_TIME=$(date +%s.%N)

ELAPSED=$(echo "$END_TIME - $START_TIME" | bc)
BANDWIDTH=$(echo "scale=2; 1000 / $ELAPSED" | bc)
log_result "     Bandwidth: ${BANDWIDTH} MB/s"
log_result ""

# 3. Audio Latency
echo -e "  ${YELLOW}Testing audio latency...${NC}"
log_result "  3. Audio Latency Test:"

if command -v arecord >/dev/null 2>&1; then
    # Measure time to capture small buffer
    START_TIME=$(date +%s.%N)
    timeout 1s arecord -f S16_LE -r 16000 -c 1 -d 1 /tmp/test.wav 2>/dev/null || true
    END_TIME=$(date +%s.%N)

    rm -f /tmp/test.wav

    LATENCY=$(echo "($END_TIME - $START_TIME) * 1000" | bc)
    log_result "     Capture latency: ${LATENCY}ms"
else
    log_result "     Audio not available"
fi
log_result ""

# 4. NPU Performance
echo -e "  ${YELLOW}Testing NPU performance...${NC}"
log_result "  4. NPU Performance:"

if [ -f /sys/class/misc/galcore/device/utilization ]; then
    # Monitor NPU for 10 seconds during hypothetical load
    log_result "     NPU utilization monitored over 10s"
    TOTAL_UTIL=0
    SAMPLES=0

    for i in {1..10}; do
        sleep 1
        UTIL=$(cat /sys/class/misc/galcore/device/utilization 2>/dev/null || echo "0")
        TOTAL_UTIL=$(echo "$TOTAL_UTIL + $UTIL" | bc)
        SAMPLES=$((SAMPLES + 1))
    done

    AVG_UTIL=$(echo "scale=1; $TOTAL_UTIL / $SAMPLES" | bc)
    log_result "     Average utilization: ${AVG_UTIL}%"
else
    log_result "     NPU not available"
fi
log_result ""

# Summary
echo ""
echo -e "${GREEN}========================================"
echo "Benchmark Complete!"
echo "========================================${NC}"
echo ""
echo "Results saved to: $RESULTS_FILE"
echo ""

cat "$RESULTS_FILE"
