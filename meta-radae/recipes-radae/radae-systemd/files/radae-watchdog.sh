#!/bin/bash
#
# RADAE Watchdog Script
# Monitors RADAE service health and restarts if needed
#

set -e

# Configuration
WATCHDOG_INTERVAL="${WATCHDOG_INTERVAL:-10}"
MAX_CPU_THRESHOLD=95
MIN_CPU_THRESHOLD=0
STUCK_THRESHOLD=30  # Seconds of no activity before considering stuck

LOG_FILE="/var/log/radae/watchdog.log"

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $*" | tee -a "$LOG_FILE"
}

# Check if main service is running
is_service_running() {
    systemctl is-active --quiet radae.service 2>/dev/null
}

# Get process ID
get_pid() {
    pgrep -x radae-trx 2>/dev/null
}

# Check if process is responsive
is_responsive() {
    local pid=$1

    # Check if process exists
    if ! kill -0 "$pid" 2>/dev/null; then
        return 1
    fi

    # Check CPU usage (too low might indicate hang)
    local cpu=$(ps -p "$pid" -o %cpu --no-headers | tr -d ' ' | cut -d. -f1)

    if [ "$cpu" -lt "$MIN_CPU_THRESHOLD" ]; then
        log "Warning: CPU usage very low ($cpu%) - possible hang"
        return 1
    fi

    # Check if process is in uninterruptible sleep (D state)
    local state=$(ps -p "$pid" -o state --no-headers | tr -d ' ')
    if [ "$state" = "D" ]; then
        log "Warning: Process in uninterruptible sleep"
        return 1
    fi

    return 0
}

# Check NPU health (if available)
check_npu() {
    if [ -f /sys/class/misc/galcore/device/driver/version ]; then
        local version=$(cat /sys/class/misc/galcore/device/driver/version 2>/dev/null)
        if [ -z "$version" ]; then
            log "Warning: NPU driver not responding"
            return 1
        fi
    fi
    return 0
}

# Check audio devices
check_audio() {
    if ! aplay -l >/dev/null 2>&1; then
        log "Error: Audio subsystem not available"
        return 1
    fi
    return 0
}

# Restart service
restart_service() {
    log "Restarting RADAE service due to health check failure"
    systemctl restart radae.service
    sleep 5
}

# Main watchdog loop
log "RADAE watchdog started (interval: ${WATCHDOG_INTERVAL}s)"

last_activity=0
stuck_count=0

while true; do
    sleep "$WATCHDOG_INTERVAL"

    # Check if service should be running
    if ! is_service_running; then
        log "Service not running, skipping checks"
        continue
    fi

    pid=$(get_pid)

    if [ -z "$pid" ]; then
        log "Error: RADAE process not found, restarting service"
        restart_service
        continue
    fi

    # Health checks
    health_ok=true

    # Check process responsiveness
    if ! is_responsive "$pid"; then
        log "Error: Process not responsive (PID: $pid)"
        health_ok=false
    fi

    # Check NPU health
    if ! check_npu; then
        log "Warning: NPU health check failed"
        # Don't restart for NPU issues, might fall back to CPU
    fi

    # Check audio devices
    if ! check_audio; then
        log "Error: Audio device check failed"
        health_ok=false
    fi

    # Check log file for errors
    if [ -f /var/log/radae/radae.log ]; then
        recent_errors=$(tail -n 50 /var/log/radae/radae.log | grep -c "ERROR" || true)
        if [ "$recent_errors" -gt 10 ]; then
            log "Warning: High error rate detected ($recent_errors errors in last 50 lines)"
        fi
    fi

    # Restart if unhealthy
    if [ "$health_ok" = false ]; then
        stuck_count=$((stuck_count + 1))

        if [ "$stuck_count" -ge 3 ]; then
            log "Health check failed $stuck_count times, restarting"
            restart_service
            stuck_count=0
        fi
    else
        stuck_count=0
    fi

    # Log status periodically
    if [ $((SECONDS % 300)) -lt "$WATCHDOG_INTERVAL" ]; then
        local cpu=$(ps -p "$pid" -o %cpu --no-headers | tr -d ' ')
        local mem=$(ps -p "$pid" -o rss --no-headers | awk '{printf "%.1f", $1/1024}')
        log "Status OK - CPU: ${cpu}%, Memory: ${mem}MB"
    fi
done
