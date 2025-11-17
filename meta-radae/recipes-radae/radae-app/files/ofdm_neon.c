/*
 * OFDM Modem with ARM NEON Optimization
 *
 * Implements OFDM modulation/demodulation for RADAE
 * Uses NEON SIMD instructions for efficient FFT and signal processing
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <complex.h>

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

#include "ofdm_neon.h"
#include "fft_neon.h"
#include "config_parser.h"

#define PI 3.14159265358979323846

int ofdm_init(ofdm_modem_t *ofdm, radae_config_t *config) {
    memset(ofdm, 0, sizeof(ofdm_modem_t));

    ofdm->sample_rate = config->ofdm_sample_rate;
    ofdm->num_carriers = config->ofdm_num_carriers;
    ofdm->symbol_rate = config->ofdm_symbol_rate;
    ofdm->cp_length = config->ofdm_cp_length;
    ofdm->pilot_spacing = config->ofdm_pilot_spacing;

    // Calculate OFDM parameters
    ofdm->fft_size = 64;  // Fixed FFT size for now
    ofdm->symbol_samples = ofdm->sample_rate / ofdm->symbol_rate;
    ofdm->cp_samples = (int)(ofdm->cp_length * ofdm->sample_rate);

    // Allocate FFT buffers
    ofdm->fft_in = malloc(ofdm->fft_size * sizeof(complex float));
    ofdm->fft_out = malloc(ofdm->fft_size * sizeof(complex float));

    if (!ofdm->fft_in || !ofdm->fft_out) {
        fprintf(stderr, "Failed to allocate OFDM buffers\n");
        return -1;
    }

    // Initialize pilot sequence
    for (int i = 0; i < MAX_CARRIERS; i++) {
        ofdm->pilots[i] = cexpf(I * 2.0f * PI * i / MAX_CARRIERS);
    }

    printf("OFDM initialized: Fs=%d Hz, Nc=%d, Rs=%d sym/s, FFT=%d\n",
           ofdm->sample_rate, ofdm->num_carriers, ofdm->symbol_rate, ofdm->fft_size);

    return 0;
}

// Map latent vector to OFDM carriers
static void map_to_carriers(ofdm_modem_t *ofdm, const float *data, int data_len,
                            complex float *carriers) {
    // Simple mapping: data values to QPSK symbols
    // In production, would use proper constellation mapping
    for (int i = 0; i < ofdm->num_carriers && i < data_len / 2; i++) {
        float re = data[i * 2];
        float im = data[i * 2 + 1];

        // QPSK mapping with normalization
        re = (re > 0) ? 1.0f : -1.0f;
        im = (im > 0) ? 1.0f : -1.0f;

        carriers[i] = (re + I * im) / sqrtf(2.0f);
    }

    // Add pilots
    for (int i = 0; i < ofdm->num_carriers; i += ofdm->pilot_spacing) {
        carriers[i] = ofdm->pilots[i];
    }
}

int ofdm_modulate(ofdm_modem_t *ofdm, const float *data, int data_len,
                  complex float *output) {
    complex float carriers[MAX_CARRIERS] = {0};
    int output_idx = 0;

    // Map data to carriers
    map_to_carriers(ofdm, data, data_len, carriers);

    // Zero out DC and prepare FFT input
    memset(ofdm->fft_in, 0, ofdm->fft_size * sizeof(complex float));

    // Place carriers in FFT bins (centered around DC)
    int start_bin = (ofdm->fft_size - ofdm->num_carriers) / 2;
    for (int i = 0; i < ofdm->num_carriers; i++) {
        ofdm->fft_in[start_bin + i] = carriers[i];
    }

    // IFFT to generate time-domain symbol
    fft_neon_inverse(ofdm->fft_in, ofdm->fft_out, ofdm->fft_size);

    // Add cyclic prefix
    for (int i = 0; i < ofdm->cp_samples; i++) {
        output[output_idx++] = ofdm->fft_out[ofdm->fft_size - ofdm->cp_samples + i];
    }

    // Copy symbol
    for (int i = 0; i < ofdm->fft_size; i++) {
        output[output_idx++] = ofdm->fft_out[i];
    }

    // Apply windowing to reduce spectral leakage
#ifdef __ARM_NEON
    // NEON-optimized windowing
    int window_len = 8;  // Raised cosine window edges
    for (int i = 0; i < window_len && i < output_idx; i++) {
        float window_val = 0.5f * (1.0f - cosf(PI * i / window_len));
        float32x2_t win = vdup_n_f32(window_val);

        // Apply window to complex sample (interleaved re/im)
        float32x2_t sample = vcreate_f32(*(uint64_t *)&output[i]);
        sample = vmul_f32(sample, win);
        *(uint64_t *)&output[i] = vreinterpret_u64_f32(sample);
    }
#endif

    return output_idx;
}

// Demodulate OFDM symbol and extract data
static void extract_from_carriers(ofdm_modem_t *ofdm, const complex float *carriers,
                                  float *data, int data_len) {
    // Simple demapping from QPSK
    for (int i = 0; i < ofdm->num_carriers && i < data_len / 2; i++) {
        // Skip pilots
        if (i % ofdm->pilot_spacing == 0) {
            continue;
        }

        complex float symbol = carriers[i];

        // Demodulate QPSK
        data[i * 2] = crealf(symbol);
        data[i * 2 + 1] = cimagf(symbol);
    }
}

int ofdm_demodulate(ofdm_modem_t *ofdm, const complex float *input,
                    float *data, int data_len) {
    // Remove cyclic prefix
    const complex float *symbol_start = input + ofdm->cp_samples;

    // Copy to FFT input
    memcpy(ofdm->fft_in, symbol_start,
           ofdm->fft_size * sizeof(complex float));

    // FFT to get frequency domain
    fft_neon_forward(ofdm->fft_in, ofdm->fft_out, ofdm->fft_size);

    // Extract carriers
    complex float carriers[MAX_CARRIERS];
    int start_bin = (ofdm->fft_size - ofdm->num_carriers) / 2;

    for (int i = 0; i < ofdm->num_carriers; i++) {
        carriers[i] = ofdm->fft_out[start_bin + i];
    }

    // Channel estimation and equalization using pilots
    for (int i = 0; i < ofdm->num_carriers; i += ofdm->pilot_spacing) {
        complex float pilot_rx = carriers[i];
        complex float pilot_tx = ofdm->pilots[i];

        // Simple equalization coefficient (in production, interpolate between pilots)
        complex float eq = conjf(pilot_tx) / (cabsf(pilot_tx) * cabsf(pilot_tx) + 0.01f);

        // Apply to nearby carriers
        for (int j = i; j < i + ofdm->pilot_spacing && j < ofdm->num_carriers; j++) {
            carriers[j] *= eq;
        }
    }

    // Extract data from carriers
    extract_from_carriers(ofdm, carriers, data, data_len);

    return 0;
}

void ofdm_cleanup(ofdm_modem_t *ofdm) {
    if (ofdm->fft_in) {
        free(ofdm->fft_in);
        ofdm->fft_in = NULL;
    }

    if (ofdm->fft_out) {
        free(ofdm->fft_out);
        ofdm->fft_out = NULL;
    }
}
