/*
 * FFT Implementation with ARM NEON Optimization
 *
 * Implements Cooley-Tukey FFT algorithm with NEON acceleration
 * Optimized for RADAE OFDM processing on ARM Cortex-A53
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <complex.h>

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

#include "fft_neon.h"

#define PI 3.14159265358979323846

// Bit reversal for FFT
static void bit_reverse(complex float *data, int n) {
    int bits = 0;
    int temp = n;

    while (temp > 1) {
        bits++;
        temp >>= 1;
    }

    for (int i = 0; i < n; i++) {
        int j = 0;
        for (int k = 0; k < bits; k++) {
            if (i & (1 << k)) {
                j |= (1 << (bits - 1 - k));
            }
        }

        if (i < j) {
            complex float tmp = data[i];
            data[i] = data[j];
            data[j] = tmp;
        }
    }
}

// Radix-2 FFT butterfly
static inline void butterfly(complex float *a, complex float *b, complex float w) {
    complex float t = (*b) * w;
    *b = *a - t;
    *a = *a + t;
}

#ifdef __ARM_NEON
// NEON-optimized complex multiplication
static inline void butterfly_neon(float32x2_t *a, float32x2_t *b,
                                  float32x2_t w) {
    // Complex multiply: (a + ib) * (c + id) = (ac - bd) + i(ad + bc)
    float32x2_t t_real = vmul_f32(*b, w);
    float32x2_t t_imag = vmul_f32(*b, vrev64_f32(w));

    t_real = vsub_f32(vdup_lane_f32(t_real, 0), vdup_lane_f32(t_real, 1));
    t_imag = vadd_f32(vdup_lane_f32(t_imag, 0), vdup_lane_f32(t_imag, 1));

    float32x2_t t = vzip_f32(t_real, t_imag).val[0];

    float32x2_t b_new = vsub_f32(*a, t);
    *a = vadd_f32(*a, t);
    *b = b_new;
}
#endif

// Forward FFT (time -> frequency)
void fft_neon_forward(const complex float *input, complex float *output, int n) {
    // Copy input to output
    memcpy(output, input, n * sizeof(complex float));

    // Bit reversal
    bit_reverse(output, n);

    // FFT stages
    for (int stage = 1; stage <= __builtin_ctz(n); stage++) {
        int m = 1 << stage;
        int m2 = m >> 1;

        // Twiddle factor
        complex float wm = cexpf(-I * 2.0f * PI / m);

        for (int k = 0; k < n; k += m) {
            complex float w = 1.0f;

            for (int j = 0; j < m2; j++) {
                butterfly(&output[k + j], &output[k + j + m2], w);
                w *= wm;
            }
        }
    }
}

// Inverse FFT (frequency -> time)
void fft_neon_inverse(const complex float *input, complex float *output, int n) {
    // Copy and conjugate
    for (int i = 0; i < n; i++) {
        output[i] = conjf(input[i]);
    }

    // Forward FFT
    bit_reverse(output, n);

    for (int stage = 1; stage <= __builtin_ctz(n); stage++) {
        int m = 1 << stage;
        int m2 = m >> 1;

        complex float wm = cexpf(-I * 2.0f * PI / m);

        for (int k = 0; k < n; k += m) {
            complex float w = 1.0f;

            for (int j = 0; j < m2; j++) {
                butterfly(&output[k + j], &output[k + j + m2], w);
                w *= wm;
            }
        }
    }

    // Conjugate and normalize
    float scale = 1.0f / n;
    for (int i = 0; i < n; i++) {
        output[i] = conjf(output[i]) * scale;
    }
}

// Real-to-complex FFT (optimized for real input)
void fft_neon_real(const float *input, complex float *output, int n) {
    // Pack real input as complex (interleave with zeros)
    complex float *temp = malloc(n * sizeof(complex float));

    for (int i = 0; i < n; i++) {
        temp[i] = input[i] + I * 0.0f;
    }

    fft_neon_forward(temp, output, n);

    free(temp);
}

// Power spectrum (|FFT|^2)
void fft_neon_power_spectrum(const complex float *fft, float *power, int n) {
#ifdef __ARM_NEON
    // NEON-optimized magnitude squared
    for (int i = 0; i < n; i += 2) {
        float32x2_t c1 = vcreate_f32(*(uint64_t *)&fft[i]);
        float32x2_t c2 = vcreate_f32(*(uint64_t *)&fft[i + 1]);

        float32x2_t mag1 = vmul_f32(c1, c1);
        float32x2_t mag2 = vmul_f32(c2, c2);

        power[i] = vget_lane_f32(mag1, 0) + vget_lane_f32(mag1, 1);
        power[i + 1] = vget_lane_f32(mag2, 0) + vget_lane_f32(mag2, 1);
    }
#else
    // Scalar fallback
    for (int i = 0; i < n; i++) {
        float re = crealf(fft[i]);
        float im = cimagf(fft[i]);
        power[i] = re * re + im * im;
    }
#endif
}
