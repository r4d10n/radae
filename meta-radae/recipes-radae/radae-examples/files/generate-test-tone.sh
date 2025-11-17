#!/bin/bash
#
# Generate Test Audio Tones for RADAE Testing
# Creates various test signals for audio validation
#

set -e

OUTPUT_DIR="${1:-/tmp}"
SAMPLE_RATE=16000
DURATION=5

echo "Generating test tones..."
echo "Output directory: $OUTPUT_DIR"
echo "Sample rate: $SAMPLE_RATE Hz"
echo "Duration: $DURATION seconds"
echo ""

# Check for sox
if ! command -v sox >/dev/null 2>&1; then
    echo "Warning: sox not found, will use alternative method"
    USE_SOX=0
else
    USE_SOX=1
fi

# 1. Sine wave (1kHz)
echo "1. Generating 1kHz sine wave..."
if [ $USE_SOX -eq 1 ]; then
    sox -n -r $SAMPLE_RATE -c 1 -b 16 "$OUTPUT_DIR/test-1khz-sine.wav" \
        synth $DURATION sine 1000 vol 0.8
else
    # Fallback: use speaker-test
    speaker-test -t sine -f 1000 -c 1 -r $SAMPLE_RATE -d $DURATION \
        -w "$OUTPUT_DIR/test-1khz-sine.wav" 2>/dev/null || \
        echo "   (skipped - no generator available)"
fi

# 2. Frequency sweep
echo "2. Generating frequency sweep (300Hz-3400Hz)..."
if [ $USE_SOX -eq 1 ]; then
    sox -n -r $SAMPLE_RATE -c 1 -b 16 "$OUTPUT_DIR/test-sweep.wav" \
        synth $DURATION sine 300-3400 vol 0.8
else
    echo "   (requires sox)"
fi

# 3. Pink noise
echo "3. Generating pink noise..."
if [ $USE_SOX -eq 1 ]; then
    sox -n -r $SAMPLE_RATE -c 1 -b 16 "$OUTPUT_DIR/test-pink-noise.wav" \
        synth $DURATION pinknoise vol 0.5
else
    echo "   (requires sox)"
fi

# 4. White noise
echo "4. Generating white noise..."
if [ $USE_SOX -eq 1 ]; then
    sox -n -r $SAMPLE_RATE -c 1 -b 16 "$OUTPUT_DIR/test-white-noise.wav" \
        synth $DURATION whitenoise vol 0.5
else
    dd if=/dev/urandom bs=32000 count=$((DURATION * SAMPLE_RATE / 16000)) 2>/dev/null | \
        sox -t raw -r $SAMPLE_RATE -b 16 -c 1 -e signed-integer - \
        "$OUTPUT_DIR/test-white-noise.wav" 2>/dev/null || \
        echo "   (skipped)"
fi

# 5. Silence
echo "5. Generating silence..."
dd if=/dev/zero bs=$((SAMPLE_RATE * 2)) count=$DURATION 2>/dev/null | \
    sox -t raw -r $SAMPLE_RATE -b 16 -c 1 -e signed-integer - \
    "$OUTPUT_DIR/test-silence.wav" 2>/dev/null || \
    echo "   (skipped)"

# 6. DTMF tones (if sox available)
if [ $USE_SOX -eq 1 ]; then
    echo "6. Generating DTMF tones (1-9)..."
    # DTMF 1 (697Hz + 1209Hz)
    sox -n -r $SAMPLE_RATE -c 1 -b 16 "$OUTPUT_DIR/test-dtmf-1.wav" \
        synth 0.5 sine 697 sine 1209 remix 1,2 vol 0.6
fi

# 7. Two-tone test (400Hz + 1kHz)
echo "7. Generating two-tone signal..."
if [ $USE_SOX -eq 1 ]; then
    sox -n -r $SAMPLE_RATE -c 1 -b 16 "$OUTPUT_DIR/test-two-tone.wav" \
        synth $DURATION sine 400 sine 1000 remix 1,2 vol 0.6
fi

echo ""
echo "Test tones generated successfully!"
echo ""
echo "Files created in $OUTPUT_DIR:"
ls -lh "$OUTPUT_DIR"/test-*.wav 2>/dev/null || echo "No files created"
echo ""
echo "Usage examples:"
echo "  Play: aplay $OUTPUT_DIR/test-1khz-sine.wav"
echo "  Test RADAE: radae-trx --input $OUTPUT_DIR/test-1khz-sine.wav"
echo ""
