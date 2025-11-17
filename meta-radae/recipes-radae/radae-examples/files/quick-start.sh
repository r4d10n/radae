#!/bin/bash
#
# RADAE Quick Start Script
# Sets up and runs RADAE for the first time
#

set -e

BLUE='\033[0;34m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${BLUE}========================================"
echo "RADAE Quick Start"
echo "========================================${NC}"
echo ""

# Check prerequisites
echo -e "${BLUE}1. Checking prerequisites...${NC}"

# Check if RADAE is installed
if ! command -v radae-trx >/dev/null 2>&1; then
    echo -e "${RED}Error: RADAE application not found${NC}"
    echo "Please install radae-app package first"
    exit 1
fi
echo -e "${GREEN}   ✓ RADAE application installed${NC}"

# Check audio devices
if ! aplay -l >/dev/null 2>&1; then
    echo -e "${RED}Error: No audio devices found${NC}"
    exit 1
fi
echo -e "${GREEN}   ✓ Audio devices available${NC}"

# Check NPU
if [ -f /sys/class/misc/galcore/device/driver/version ]; then
    echo -e "${GREEN}   ✓ NPU available${NC}"
else
    echo -e "${YELLOW}   ⚠ NPU not found (will use CPU)${NC}"
fi

# Check models
if [ -f /usr/share/radae/radae_encoder_int8.tflite ]; then
    echo -e "${GREEN}   ✓ NPU models installed${NC}"
else
    echo -e "${YELLOW}   ⚠ NPU models not found${NC}"
fi

# Setup configuration
echo ""
echo -e "${BLUE}2. Setting up configuration...${NC}"

CONFIG_DIR="$HOME/.config/radae"
CONFIG_FILE="$CONFIG_DIR/radae.conf"

mkdir -p "$CONFIG_DIR"

if [ ! -f "$CONFIG_FILE" ]; then
    # Copy default config
    if [ -f /usr/share/doc/radae/examples/configs/radae-basic.conf ]; then
        cp /usr/share/doc/radae/examples/configs/radae-basic.conf "$CONFIG_FILE"
        echo -e "${GREEN}   ✓ Created configuration: $CONFIG_FILE${NC}"
    else
        # Create minimal config
        cat > "$CONFIG_FILE" << 'EOF'
[audio]
sample_rate = 16000
channels = 1
period_size = 160
buffer_size = 1600
capture_device = "hw:0,0"
playback_device = "hw:0,0"

[ofdm]
sample_rate = 8000
num_carriers = 30
symbol_rate = 50
cp_length = 0.002
pilot_spacing = 4

[npu]
enable = 1
encoder_model = "/usr/share/radae/radae_encoder_int8.tflite"
decoder_model = "/usr/share/radae/radae_decoder_int8.tflite"
num_threads = 2

[ptt]
enable = 0

[logging]
level = "info"
file = "$HOME/.local/log/radae.log"
EOF
        echo -e "${GREEN}   ✓ Created default configuration${NC}"
    fi
else
    echo -e "${YELLOW}   ⚠ Configuration already exists${NC}"
fi

# Create log directory
mkdir -p "$HOME/.local/log"

# Test run
echo ""
echo -e "${BLUE}3. Running quick test...${NC}"
echo -e "${YELLOW}   Starting RADAE for 5 seconds...${NC}"
echo ""

timeout 5s radae-trx --config "$CONFIG_FILE" || true

echo ""
echo -e "${GREEN}========================================"
echo "Quick start complete!"
echo "========================================${NC}"
echo ""
echo "Next steps:"
echo "  1. Edit configuration: $CONFIG_FILE"
echo "  2. Run RADAE: radae-trx --config $CONFIG_FILE"
echo "  3. Or use systemd: sudo systemctl start radae.service"
echo ""
echo "Documentation: /usr/share/doc/radae/"
echo "Examples: /usr/share/doc/radae/examples/"
echo ""
