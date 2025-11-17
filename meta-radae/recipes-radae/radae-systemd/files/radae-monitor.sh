#!/bin/bash
#
# RADAE Monitoring Script
# Display real-time status and performance metrics
#

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Check if service is running
is_running() {
    systemctl is-active --quiet radae.service 2>/dev/null
}

# Get CPU usage for RADAE process
get_cpu_usage() {
    local pid=$(pgrep -x radae-trx 2>/dev/null)
    if [ -n "$pid" ]; then
        ps -p "$pid" -o %cpu --no-headers | tr -d ' '
    else
        echo "N/A"
    fi
}

# Get memory usage
get_mem_usage() {
    local pid=$(pgrep -x radae-trx 2>/dev/null)
    if [ -n "$pid" ]; then
        ps -p "$pid" -o rss --no-headers | awk '{printf "%.1f MB", $1/1024}'
    else
        echo "N/A"
    fi
}

# Get NPU utilization (if available)
get_npu_usage() {
    if [ -f /sys/class/misc/galcore/device/utilization ]; then
        cat /sys/class/misc/galcore/device/utilization 2>/dev/null || echo "N/A"
    else
        echo "N/A"
    fi
}

# Get audio device status
get_audio_status() {
    if aplay -l 2>/dev/null | grep -q "card 0"; then
        echo -e "${GREEN}OK${NC}"
    else
        echo -e "${RED}Error${NC}"
    fi
}

# Display status
display_status() {
    clear
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}      RADAE Real-Time Monitor${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo ""

    # Service status
    if is_running; then
        echo -e "Service Status:    ${GREEN}RUNNING${NC}"
    else
        echo -e "Service Status:    ${RED}STOPPED${NC}"
    fi

    # CPU usage
    local cpu=$(get_cpu_usage)
    echo -e "CPU Usage:         ${cpu}%"

    # Memory usage
    local mem=$(get_mem_usage)
    echo -e "Memory Usage:      ${mem}"

    # NPU usage
    local npu=$(get_npu_usage)
    echo -e "NPU Utilization:   ${npu}"

    # Audio status
    echo -ne "Audio Devices:     "
    get_audio_status

    echo ""
    echo -e "${YELLOW}Recent Log Entries:${NC}"
    echo "----------------------------------------"

    if [ -f /var/log/radae/radae.log ]; then
        tail -n 10 /var/log/radae/radae.log
    else
        journalctl -u radae.service -n 10 --no-pager 2>/dev/null || echo "No logs available"
    fi

    echo ""
    echo -e "${BLUE}========================================${NC}"
    echo "Press Ctrl+C to exit"
}

# Continuous monitoring
monitor_continuous() {
    while true; do
        display_status
        sleep 2
    done
}

# One-time status display
if [ "${1:-}" = "--once" ] || [ "${1:-}" = "-1" ]; then
    display_status
else
    trap 'echo ""; exit 0' INT
    monitor_continuous
fi
