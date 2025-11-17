# recipes-support

This directory contains BitBake recipes for supporting libraries and utilities.

## Purpose

Houses recipes for libraries and tools that support RADAE operation:

- **audio-processing**: Audio processing libraries (codec2, opus, etc.)
- **dsp-utils**: Digital signal processing utilities
- **radio-tools**: Radio communication utilities
- **performance-tools**: Profiling and benchmarking tools
- **logging**: Enhanced logging frameworks
- **configuration**: Configuration management tools

## Common Support Packages

### Audio Libraries

```bitbake
SUMMARY = "Audio Processing Support for RADAE"
DEPENDS = "alsa-lib"

# Example packages:
# - libsamplerate: Sample rate conversion
# - libsndfile: Audio file I/O
# - speex: Speech processing
```

### DSP Libraries

```bitbake
SUMMARY = "DSP Utilities for RADAE"

# Example packages:
# - fftw: Fast Fourier Transform
# - liquid-dsp: DSP library for SDR
# - scipy: Python scientific computing
```

### Codec2

If using Codec2 for voice compression:

```bitbake
SUMMARY = "Codec2 Speech Codec"
HOMEPAGE = "http://www.rowetel.com/codec2.html"
LICENSE = "LGPL-2.1"

SRC_URI = "git://github.com/drowe67/codec2.git;protocol=https"

inherit cmake

EXTRA_OECMAKE = "-DUNITTEST=OFF"

RDEPENDS:${PN} = "libsamplerate libsndfile"
```

## Recipe Directory Structure

```
recipes-support/
├── audio-libs/
│   ├── codec2_0.9.2.bb
│   ├── libsamplerate_0.2.2.bb
│   └── libsndfile_1.0.31.bb
├── dsp-libs/
│   ├── fftw_3.3.10.bb
│   └── liquid-dsp_1.4.0.bb
└── radio-tools/
    ├── hamlib_4.5.bb
    └── rtl-sdr_0.6.0.bb
```

## Integration Guidelines

- Link support libraries with RADAE core
- Ensure ALSA compatibility for audio I/O
- Optimize DSP libraries with NEON instructions
- Test audio pipelines end-to-end

## Performance Optimization

Enable ARM NEON for DSP operations:
```bitbake
CFLAGS:append = " -mfpu=neon -ftree-vectorize"
EXTRA_OECMAKE += "-DENABLE_NEON=ON"
```
