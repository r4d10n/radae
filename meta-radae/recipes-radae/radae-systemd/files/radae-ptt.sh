#!/bin/bash
#
# RADAE PTT Control Script
# Manually control PTT via GPIO
#

set -e

# Configuration
CONFIG_FILE="/etc/radae/radae.conf"

# Source configuration
if [ -f "$CONFIG_FILE" ]; then
    source "$CONFIG_FILE"
fi

# Defaults
GPIO_CHIP="${PTT_GPIO_CHIP:-gpiochip0}"
GPIO_LINE="${PTT_GPIO_LINE:-12}"
ACTIVE_LOW="${PTT_ACTIVE_LOW:-0}"

usage() {
    cat << EOF
Usage: $0 <command>

Commands:
    on      Activate PTT (start transmitting)
    off     Deactivate PTT (stop transmitting)
    status  Show PTT status
    test    Test PTT (on for 2 seconds, then off)

Configuration:
    GPIO Chip: $GPIO_CHIP
    GPIO Line: $GPIO_LINE
    Active Low: $ACTIVE_LOW

EOF
    exit 1
}

# Check if gpioset is available
if ! command -v gpioset >/dev/null 2>&1; then
    echo "Error: gpioset not found. Install libgpiod-utils" >&2
    exit 1
fi

ptt_on() {
    local value=1
    [ "$ACTIVE_LOW" = "1" ] && value=0

    echo "Activating PTT on $GPIO_CHIP line $GPIO_LINE (value=$value)"
    gpioset --mode=exit "$GPIO_CHIP" "$GPIO_LINE"="$value"
}

ptt_off() {
    local value=0
    [ "$ACTIVE_LOW" = "1" ] && value=1

    echo "Deactivating PTT on $GPIO_CHIP line $GPIO_LINE (value=$value)"
    gpioset --mode=exit "$GPIO_CHIP" "$GPIO_LINE"="$value"
}

ptt_status() {
    if command -v gpioget >/dev/null 2>&1; then
        local current=$(gpioget "$GPIO_CHIP" "$GPIO_LINE" 2>/dev/null || echo "unknown")
        local is_active=0

        if [ "$ACTIVE_LOW" = "1" ]; then
            [ "$current" = "0" ] && is_active=1
        else
            [ "$current" = "1" ] && is_active=1
        fi

        echo "PTT Status:"
        echo "  GPIO: $GPIO_CHIP line $GPIO_LINE"
        echo "  Current value: $current"
        echo "  Active: $([ $is_active -eq 1 ] && echo 'YES' || echo 'NO')"
        echo "  Active low: $ACTIVE_LOW"
    else
        echo "Cannot read GPIO status (gpioget not available)"
    fi
}

ptt_test() {
    echo "Testing PTT..."
    ptt_on
    sleep 2
    ptt_off
    echo "Test complete"
}

# Main
case "${1:-}" in
    on)
        ptt_on
        ;;
    off)
        ptt_off
        ;;
    status)
        ptt_status
        ;;
    test)
        ptt_test
        ;;
    *)
        usage
        ;;
esac
