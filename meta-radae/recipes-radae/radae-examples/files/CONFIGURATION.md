# RADAE Configuration Guide

Complete guide to configuring RADAE for optimal performance on i.MX 8M Plus.

## Configuration File Format

RADAE uses INI-style configuration files with sections and key-value pairs:

```ini
[section]
key = value
```

## Configuration Locations

RADAE searches for configuration in this order:

1. Command-line specified: `--config /path/to/file.conf`
2. User configuration: `~/.config/radae/radae.conf`
3. System configuration: `/etc/radae/radae.conf`

## Configuration Sections

### [audio] - Audio Configuration

Controls ALSA audio capture and playback.

```ini
[audio]
sample_rate = 16000
channels = 1
period_size = 160
buffer_size = 1600
capture_device = "hw:0,0"
playback_device = "hw:0,0"
```

#### sample_rate
- **Type**: Integer
- **Default**: 16000
- **Range**: 8000-48000
- **Description**: Audio sample rate in Hz
- **Recommendations**:
  - 16000: Standard for RADAE
  - 8000: Lower quality, reduced latency
  - 48000: Higher quality, increased latency

#### channels
- **Type**: Integer
- **Default**: 1
- **Values**: 1 (mono), 2 (stereo)
- **Description**: Number of audio channels
- **Note**: RADAE uses mono only

#### period_size
- **Type**: Integer
- **Default**: 160
- **Range**: 80-640
- **Description**: ALSA period size in frames
- **Latency Impact**:
  - 80 (5ms @ 16kHz): Low latency, high CPU
  - 160 (10ms @ 16kHz): Balanced
  - 320 (20ms @ 16kHz): Higher latency, lower CPU

#### buffer_size
- **Type**: Integer
- **Default**: 1600
- **Range**: 800-6400
- **Description**: ALSA buffer size in frames
- **Recommendations**: 10× period_size for stability

#### capture_device / playback_device
- **Type**: String
- **Default**: "hw:0,0"
- **Description**: ALSA device identifier
- **Format**: "hw:CARD,DEVICE"
- **Find devices**: `aplay -l` / `arecord -l`

### [ofdm] - OFDM Modem Configuration

Controls OFDM modulation parameters.

```ini
[ofdm]
sample_rate = 8000
num_carriers = 30
symbol_rate = 50
cp_length = 0.002
pilot_spacing = 4
```

#### sample_rate
- **Type**: Integer
- **Default**: 8000
- **Description**: OFDM symbol rate in Hz
- **Note**: Typically half of audio sample rate

#### num_carriers
- **Type**: Integer
- **Default**: 30
- **Range**: 10-64
- **Description**: Number of OFDM carriers
- **Tradeoff**:
  - More carriers: Higher data rate, more sensitive to Doppler
  - Fewer carriers: Lower data rate, more robust

#### symbol_rate
- **Type**: Integer
- **Default**: 50
- **Range**: 25-100
- **Description**: OFDM symbols per second
- **Impact**:
  - Higher: Lower latency, less robust
  - Lower: Higher latency, more robust

#### cp_length
- **Type**: Float
- **Default**: 0.002
- **Range**: 0.001-0.010
- **Description**: Cyclic prefix length in seconds
- **Purpose**: Guard interval for multipath

#### pilot_spacing
- **Type**: Integer
- **Default**: 4
- **Range**: 2-8
- **Description**: Spacing between pilot carriers
- **Tradeoff**:
  - Smaller: Better channel estimation, less data
  - Larger: More data, worse channel estimation

### [npu] - NPU Configuration

Controls NPU acceleration settings.

```ini
[npu]
enable = 1
encoder_model = "/usr/share/radae/radae_encoder_int8.tflite"
decoder_model = "/usr/share/radae/radae_decoder_int8.tflite"
num_threads = 2
```

#### enable
- **Type**: Boolean
- **Default**: 1
- **Values**: 0 (disabled), 1 (enabled)
- **Description**: Enable NPU acceleration
- **Note**: Falls back to CPU if NPU unavailable

#### encoder_model / decoder_model
- **Type**: String
- **Description**: Path to TFLite model files
- **Note**: INT8 quantized models for NPU

#### num_threads
- **Type**: Integer
- **Default**: 2
- **Range**: 1-4
- **Description**: CPU threads for TFLite inference
- **Recommendations**:
  - 2: Balanced (NPU + light CPU)
  - 4: Maximum performance (all cores)
  - 1: Minimum overhead

### [ptt] - PTT Control Configuration

Controls GPIO-based PTT.

```ini
[ptt]
enable = 0
gpio_chip = "gpiochip0"
gpio_line = 12
active_low = 0
delay_ms = 100
```

#### enable
- **Type**: Boolean
- **Default**: 0
- **Description**: Enable PTT control

#### gpio_chip
- **Type**: String
- **Default**: "gpiochip0"
- **Description**: GPIO chip name
- **Find chips**: `gpiodetect`

#### gpio_line
- **Type**: Integer
- **Default**: 12
- **Description**: GPIO line number
- **Find lines**: `gpioinfo`

#### active_low
- **Type**: Boolean
- **Default**: 0
- **Values**: 0 (active high), 1 (active low)
- **Description**: PTT polarity

#### delay_ms
- **Type**: Integer
- **Default**: 100
- **Range**: 0-1000
- **Description**: PTT activation delay in milliseconds
- **Purpose**: Allow radio to stabilize before transmission

### [logging] - Logging Configuration

Controls logging behavior.

```ini
[logging]
level = "info"
file = "/var/log/radae/radae.log"
```

#### level
- **Type**: String
- **Default**: "info"
- **Values**: debug, info, warning, error
- **Description**: Logging verbosity

#### file
- **Type**: String
- **Default**: "/var/log/radae/radae.log"
- **Description**: Log file path

## Performance Tuning

### Low Latency Configuration

Minimize end-to-end latency:

```ini
[audio]
period_size = 80      # 5ms periods
buffer_size = 800     # Smaller buffer

[ofdm]
symbol_rate = 100     # Faster symbols
cp_length = 0.001     # Shorter CP

[npu]
num_threads = 4       # Use all cores
```

**Expected latency**: ~150ms

### Low Power Configuration

Maximize battery life:

```ini
[audio]
period_size = 320     # 20ms periods (less wakeups)
buffer_size = 3200

[ofdm]
num_carriers = 20     # Fewer carriers
symbol_rate = 40      # Lower rate

[npu]
num_threads = 2       # Fewer threads
```

**Expected power**: 2-3W

### High Quality Configuration

Maximize audio quality:

```ini
[audio]
sample_rate = 48000   # Higher sample rate
period_size = 240

[ofdm]
num_carriers = 40     # More carriers
symbol_rate = 50

[npu]
num_threads = 4
```

## System Integration

### CPU Governor

For best performance, set CPU governor to performance:

```bash
# In /etc/radae/radae-env
CPU_GOVERNOR=performance
```

Or manually:

```bash
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
```

### Real-time Priority

For low latency, use real-time scheduling:

```bash
# In systemd service (already configured)
LimitRTPRIO=95
Nice=-10
```

### Memory Locking

Prevent swapping for consistent performance:

```bash
# In systemd service (already configured)
LimitMEMLOCK=infinity
```

## Validation

Test configuration before deployment:

```bash
# Validate syntax
radae-trx --config myconfig.conf --check

# Test run (verbose)
radae-trx --config myconfig.conf --verbose

# Monitor performance
radae-monitor.sh
```

## Example Configurations

See `/usr/share/doc/radae/examples/configs/` for:

- `radae-basic.conf` - Balanced settings
- `radae-low-latency.conf` - Minimum latency
- `radae-low-power.conf` - Battery operation
- `radae-test.conf` - Debug/testing
