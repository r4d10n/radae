/*
 * FFT with NEON Optimization Header
 */

#ifndef FFT_NEON_H
#define FFT_NEON_H

#include <complex.h>

/**
 * Forward FFT (time -> frequency domain)
 * @param input Input samples (time domain)
 * @param output Output samples (frequency domain)
 * @param n FFT size (must be power of 2)
 */
void fft_neon_forward(const complex float *input, complex float *output, int n);

/**
 * Inverse FFT (frequency -> time domain)
 * @param input Input samples (frequency domain)
 * @param output Output samples (time domain)
 * @param n FFT size (must be power of 2)
 */
void fft_neon_inverse(const complex float *input, complex float *output, int n);

/**
 * Real-to-complex FFT
 * @param input Real input samples
 * @param output Complex output samples
 * @param n FFT size (must be power of 2)
 */
void fft_neon_real(const float *input, complex float *output, int n);

/**
 * Compute power spectrum
 * @param fft FFT output
 * @param power Power spectrum output
 * @param n FFT size
 */
void fft_neon_power_spectrum(const complex float *fft, float *power, int n);

#endif /* FFT_NEON_H */
