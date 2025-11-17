#!/bin/bash
#
# RADAE Audio Loopback Test
# Tests audio capture and playback functionality
#

set -e

echo "==================================="
echo "RADAE Audio Loopback Test"
echo "==================================="
echo ""

# Check if audio devices are available
echo "1. Checking audio devices..."
if ! aplay -l >/dev/null 2>&1; then
    echo "Error: No audio playback devices found"
    exit 1
fi

if ! arecord -l >/dev/null 2>&1; then
    echo "Error: No audio capture devices found"
    exit 1
fi

echo "   Playback devices:"
aplay -l | grep "^card"

echo ""
echo "   Capture devices:"
arecord -l | grep "^card"

# Test simple loopback
echo ""
echo "2. Testing audio loopback (5 seconds)..."
echo "   Speak into the microphone, you should hear your voice"

timeout 5s arecord -f S16_LE -r 16000 -c 1 | aplay -f S16_LE -r 16000 -c 1 || true

echo ""
echo "3. Testing with file recording..."

# Record test audio
TEMP_FILE=$(mktemp /tmp/radae-test-XXXXXX.wav)
echo "   Recording 3 seconds to $TEMP_FILE..."
arecord -f S16_LE -r 16000 -c 1 -d 3 "$TEMP_FILE"

echo "   Playing back..."
aplay "$TEMP_FILE"

# Cleanup
rm -f "$TEMP_FILE"

echo ""
echo "Audio loopback test complete!"
echo "==================================="
