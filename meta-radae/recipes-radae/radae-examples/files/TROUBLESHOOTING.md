# RADAE Troubleshooting Guide

Common issues and solutions for RADAE on i.MX 8M Plus.

## Audio Issues

### No Audio Devices Found

**Symptom**: `Error: No audio playback devices found`

**Diagnosis**:
```bash
# Check if audio devices exist
aplay -l
arecord -l

# Check if ALSA modules are loaded
lsmod | grep snd
```

**Solutions**:

1. Load audio drivers:
```bash
sudo modprobe snd_soc_imx_card
```

2. Check device tree configuration (hardware-specific)

3. Verify audio codec is detected:
```bash
cat /proc/asound/cards
```

### Audio Dropouts / Underruns

**Symptom**: Clicking sounds, gaps in audio, "underrun" errors in logs

**Diagnosis**:
```bash
# Check system load
top

# Check audio latency
cat /proc/asound/card0/pcm0p/sub0/hw_params

# Monitor ALSA errors
journalctl -u radae.service | grep underrun
```

**Solutions**:

1. Increase buffer size in configuration:
```ini
[audio]
period_size = 320
buffer_size = 3200
```

2. Set CPU to performance mode:
```bash
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
```

3. Increase process priority (already done in systemd service)

4. Disable power management:
```bash
# In /boot/boot.txt or kernel command line
nohz=off intel_idle.max_cstate=0
```

### Wrong Audio Device

**Symptom**: No sound, wrong device being used

**Diagnosis**:
```bash
# List all audio devices
aplay -L
arecord -L
```

**Solutions**:

1. Update configuration with correct device:
```ini
[audio]
capture_device = "hw:1,0"  # Change card number
playback_device = "hw:1,0"
```

2. Use device names instead of numbers:
```ini
capture_device = "plughw:CARD=imx8mpevk,DEV=0"
```

3. Set default device in `~/.asoundrc`:
```
defaults.pcm.card 1
defaults.ctl.card 1
```

## NPU Issues

### NPU Not Found

**Symptom**: `Warning: NPU device not found`

**Diagnosis**:
```bash
# Check NPU device
ls -l /dev/galcore

# Check NPU driver
cat /sys/class/misc/galcore/device/driver/version

# Check kernel modules
lsmod | grep galcore
```

**Solutions**:

1. Load NPU driver:
```bash
sudo modprobe galcore
```

2. Verify NPU is enabled in device tree

3. Check BSP includes NPU support:
```bash
# Should show NPU driver version
cat /sys/class/misc/galcore/device/driver/version
```

4. Fallback to CPU:
```ini
[npu]
enable = 0  # Disable NPU, use CPU
```

### NPU Models Not Found

**Symptom**: `Error: Encoder model not found`

**Diagnosis**:
```bash
# Check if models exist
ls -lh /usr/share/radae/*.tflite

# Check package installation
dpkg -L radae-npu | grep tflite
```

**Solutions**:

1. Install NPU models package:
```bash
sudo apt install radae-npu
```

2. Manually download models:
```bash
sudo mkdir -p /usr/share/radae
cd /usr/share/radae
# Download models from release
```

3. Update model paths in configuration:
```ini
[npu]
encoder_model = "/path/to/encoder.tflite"
decoder_model = "/path/to/decoder.tflite"
```

### Low NPU Performance

**Symptom**: NPU utilization low, CPU usage high

**Diagnosis**:
```bash
# Monitor NPU utilization
watch -n1 cat /sys/class/misc/galcore/device/utilization

# Check if NPU delegate is loading
journalctl -u radae.service | grep -i npu
```

**Solutions**:

1. Verify NPU delegate is installed:
```bash
ls -l /usr/lib/libvsi_npu.so
```

2. Check model is INT8 quantized (required for NPU)

3. Increase NPU clock frequency (if available):
```bash
cat /sys/class/misc/galcore/device/clk
```

4. Ensure models are optimized for NPU

## PTT Issues

### PTT Not Working

**Symptom**: PTT command has no effect

**Diagnosis**:
```bash
# Check GPIO availability
gpiodetect
gpioinfo

# Check permissions
ls -l /dev/gpiochip*

# Test GPIO manually
gpioset gpiochip0 12=1
gpioget gpiochip0 12
```

**Solutions**:

1. Add user to gpio group:
```bash
sudo usermod -a -G gpio radae
```

2. Set GPIO permissions:
```bash
sudo chmod 666 /dev/gpiochip0
```

3. Use correct GPIO chip/line:
```bash
# Find correct GPIO
gpioinfo | grep -A2 "line.*PTT"
```

4. Check polarity:
```ini
[ptt]
active_low = 1  # Try opposite value
```

### GPIO Permission Denied

**Symptom**: `Error: Failed to open GPIO chip`

**Solutions**:

1. Run with sudo (temporary):
```bash
sudo radae-ptt.sh on
```

2. Create udev rule (permanent):
```bash
# /etc/udev/rules.d/99-gpio.rules
SUBSYSTEM=="gpio", KERNEL=="gpiochip*", MODE="0666"
```

3. Reload udev:
```bash
sudo udevadm control --reload-rules
sudo udevadm trigger
```

## Service Issues

### Service Won't Start

**Symptom**: `systemctl start radae.service` fails

**Diagnosis**:
```bash
# Check service status
systemctl status radae.service

# View detailed logs
journalctl -u radae.service -n 50

# Check configuration syntax
radae-trx --config /etc/radae/radae.conf --check
```

**Solutions**:

1. Fix configuration errors

2. Check file permissions:
```bash
sudo chown radae:radae /var/log/radae
sudo chmod 755 /var/log/radae
```

3. Verify binary exists:
```bash
which radae-trx
ldd $(which radae-trx)  # Check dependencies
```

4. Run manually for debugging:
```bash
radae-trx --config /etc/radae/radae.conf --verbose
```

### Service Crashes / Restarts

**Symptom**: Service keeps restarting, shown in `systemctl status`

**Diagnosis**:
```bash
# Check crash logs
journalctl -u radae.service | grep -i "error\|segfault\|crash"

# Check system resources
free -h
df -h

# Monitor in real-time
journalctl -u radae.service -f
```

**Solutions**:

1. Check memory usage:
```bash
# Increase swap if needed
sudo fallocate -l 2G /swapfile
sudo mkswap /swapfile
sudo swapon /swapfile
```

2. Reduce resource usage:
```ini
[npu]
num_threads = 1  # Use fewer threads

[audio]
buffer_size = 800  # Smaller buffers
```

3. Check for hardware issues:
```bash
dmesg | grep -i error
```

## Performance Issues

### High Latency

**Symptom**: Noticeable delay in audio

**Diagnosis**:
```bash
# Check processing time
journalctl -u radae.service | grep "processing time"

# Monitor CPU usage
top -p $(pgrep radae-trx)

# Check audio latency
cat /proc/asound/card0/pcm0p/sub0/hw_params
```

**Solutions**:

1. Use low-latency configuration:
```bash
cp /usr/share/doc/radae/examples/configs/radae-low-latency.conf \
   /etc/radae/radae.conf
```

2. Reduce buffer sizes:
```ini
[audio]
period_size = 80
buffer_size = 800
```

3. Enable NPU:
```ini
[npu]
enable = 1
num_threads = 4
```

4. Set real-time priority:
```bash
sudo chrt -f -p 80 $(pgrep radae-trx)
```

### High CPU Usage

**Symptom**: CPU usage >90%, system sluggish

**Diagnosis**:
```bash
# Check which component is using CPU
perf top -p $(pgrep radae-trx)

# Check if NPU is being used
cat /sys/class/misc/galcore/device/utilization
```

**Solutions**:

1. Verify NPU is enabled and working

2. Increase buffer sizes (reduce wakeup frequency):
```ini
[audio]
period_size = 320
buffer_size = 3200
```

3. Reduce logging:
```ini
[logging]
level = "error"
```

### High Power Consumption

**Symptom**: Battery drains quickly, high temperature

**Solutions**:

1. Use low-power configuration:
```bash
cp /usr/share/doc/radae/examples/configs/radae-low-power.conf \
   /etc/radae/radae.conf
```

2. Set CPU governor to powersave:
```bash
echo powersave | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
```

3. Reduce NPU workload:
```ini
[ofdm]
num_carriers = 20
symbol_rate = 40
```

## Log Analysis

### Finding Errors

```bash
# Recent errors
journalctl -u radae.service -p err -n 50

# Errors in last hour
journalctl -u radae.service -p err --since "1 hour ago"

# Follow errors in real-time
journalctl -u radae.service -p err -f
```

### Debug Logging

Enable debug logging temporarily:

```bash
# Edit config
sudo vi /etc/radae/radae.conf
# Set: level = "debug"

# Restart service
sudo systemctl restart radae.service

# Watch logs
journalctl -u radae.service -f

# Don't forget to disable debug when done!
```

## Getting Help

If issues persist:

1. **Collect diagnostic information**:
```bash
# Run comprehensive benchmark
/usr/share/doc/radae/examples/scripts/benchmark.sh

# Save system information
uname -a > /tmp/radae-debug.txt
journalctl -u radae.service -n 100 >> /tmp/radae-debug.txt
dmesg | tail -100 >> /tmp/radae-debug.txt
```

2. **Check GitHub Issues**:
   - Search existing issues: https://github.com/drowe67/radae/issues
   - Create new issue with diagnostic info

3. **Provide Configuration**:
```bash
cat /etc/radae/radae.conf
```

4. **Include Hardware Details**:
   - i.MX 8M Plus variant
   - BSP version
   - Custom hardware modifications

## Recovery

### Reset to Defaults

```bash
# Stop service
sudo systemctl stop radae.service

# Backup current config
sudo cp /etc/radae/radae.conf /etc/radae/radae.conf.backup

# Restore default config
sudo cp /usr/share/doc/radae/examples/configs/radae-basic.conf \
        /etc/radae/radae.conf

# Clear logs
sudo truncate -s 0 /var/log/radae/radae.log

# Restart
sudo systemctl start radae.service
```

### Complete Reinstall

```bash
# Stop and disable service
sudo systemctl stop radae.service
sudo systemctl disable radae.service

# Remove packages
sudo apt remove --purge radae-app radae-systemd radae-npu

# Clean configuration
sudo rm -rf /etc/radae /var/log/radae /var/lib/radae

# Reinstall
sudo apt install radae-app radae-systemd radae-npu

# Reconfigure
sudo systemctl enable radae.service
sudo systemctl start radae.service
```
