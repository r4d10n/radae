# RADAE for i.MX 8M Plus

Real-time Artificial Intelligence Deep learning Audio Encoder (RADAE) optimized for NXP i.MX 8M Plus with NPU acceleration.

## Overview

RADAE is a neural vocoder for HF radio voice communication that provides high-quality speech compression using machine learning. This implementation is optimized for the i.MX 8M Plus platform, leveraging its 2.3 TOPS NPU for accelerated inference.

## Features

- **NPU Acceleration**: 8-10× real-time performance using VeriSilicon NPU
- **Low Latency**: ~185ms end-to-end (120ms algorithmic + 65ms processing)
- **Power Efficient**: 3-5W total system power (vs 8-10W CPU-only)
- **Full-Duplex**: Simultaneous TX and RX operation
- **ALSA Integration**: Low-latency audio capture and playback
- **OFDM Modem**: NEON-optimized modulation/demodulation
- **PTT Control**: GPIO-based push-to-talk
- **Systemd Service**: Auto-start and watchdog support

## Quick Start

### 1. Installation

The RADAE packages should already be installed on your system:

```bash
# Check installation
which radae-trx
systemctl status radae.service
```

### 2. First Run

Use the quick-start script:

```bash
/usr/share/doc/radae/examples/scripts/quick-start.sh
```

This will:
- Check prerequisites (audio, NPU, models)
- Create user configuration
- Run a quick test

### 3. Manual Configuration

Edit the configuration file:

```bash
vi ~/.config/radae/radae.conf
```

Or use system-wide configuration:

```bash
sudo vi /etc/radae/radae.conf
```

### 4. Run RADAE

As user application:

```bash
radae-trx --config ~/.config/radae/radae.conf
```

As systemd service:

```bash
sudo systemctl start radae.service
sudo systemctl status radae.service
```

## Applications

The RADAE suite includes three applications:

### radae-trx (Full-Duplex)

Simultaneous transmit and receive:

```bash
radae-trx --config /etc/radae/radae.conf
```

### radae-tx (Transmit Only)

Transmit-only mode for testing:

```bash
radae-tx --input test.wav --output tx.iq
```

### radae-rx (Receive Only)

Receive-only mode:

```bash
radae-rx --input rx.iq --output audio.wav
```

## Configuration Profiles

Example configurations are provided in `/usr/share/doc/radae/examples/configs/`:

- **radae-basic.conf**: Balanced settings for general use
- **radae-low-latency.conf**: Optimized for minimum latency
- **radae-low-power.conf**: Optimized for battery operation
- **radae-test.conf**: Debug and testing configuration

Copy and modify these for your needs:

```bash
cp /usr/share/doc/radae/examples/configs/radae-low-latency.conf \
   ~/.config/radae/radae.conf
```

## Testing

### Audio Loopback Test

```bash
/usr/share/doc/radae/examples/scripts/test-audio-loopback.sh
```

### NPU Performance Test

```bash
/usr/share/doc/radae/examples/scripts/test-npu-performance.sh
```

### Full Benchmark

```bash
/usr/share/doc/radae/examples/scripts/benchmark.sh
```

### Generate Test Tones

```bash
/usr/share/doc/radae/examples/scripts/generate-test-tone.sh /tmp
aplay /tmp/test-1khz-sine.wav
```

## Monitoring

### Real-time Monitor

```bash
/usr/sbin/radae-monitor.sh
```

Shows:
- Service status
- CPU/Memory usage
- NPU utilization
- Recent log entries

### Manual Monitoring

```bash
# Service status
systemctl status radae.service

# Logs
journalctl -u radae.service -f

# NPU utilization
cat /sys/class/misc/galcore/device/utilization

# CPU usage
top -p $(pgrep radae-trx)
```

## PTT Control

### Enable PTT

Edit configuration:

```conf
[ptt]
enable = 1
gpio_chip = "gpiochip0"
gpio_line = 12
active_low = 0
delay_ms = 100
```

### Manual PTT Control

```bash
# Activate PTT
sudo /usr/sbin/radae-ptt.sh on

# Deactivate PTT
sudo /usr/sbin/radae-ptt.sh off

# Check status
sudo /usr/sbin/radae-ptt.sh status

# Test (2 second pulse)
sudo /usr/sbin/radae-ptt.sh test
```

## Performance

Expected performance on i.MX 8M Plus:

| Metric | Value |
|--------|-------|
| Real-time Factor | 8-10× |
| End-to-End Latency | ~185ms |
| System Power | 3-5W |
| CPU Usage | 25-40% |
| NPU Utilization | 40-60% |
| Memory Usage | 150-250 MB |

## Troubleshooting

See [TROUBLESHOOTING.md](TROUBLESHOOTING.md) for common issues and solutions.

## Documentation

- [CONFIGURATION.md](CONFIGURATION.md) - Detailed configuration guide
- [TROUBLESHOOTING.md](TROUBLESHOOTING.md) - Common issues and fixes
- `man radae` - Command-line reference

## Project Links

- **Source Code**: https://github.com/drowe67/radae
- **Documentation**: https://github.com/drowe67/radae/blob/main/README.md
- **Issues**: https://github.com/drowe67/radae/issues

## License

RADAE is licensed under the MIT License. See LICENSE file for details.

## Support

For questions and support:
- GitHub Issues: https://github.com/drowe67/radae/issues
- Mailing List: (if available)
