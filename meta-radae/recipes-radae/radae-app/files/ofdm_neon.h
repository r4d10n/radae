/*
 * OFDM Modem Header
 */

#ifndef OFDM_NEON_H
#define OFDM_NEON_H

#include <complex.h>

#define MAX_CARRIERS 64
#define LATENT_SIZE 80
#define OFDM_SAMPLES 960  // Approximately 120ms @ 8kHz

typedef struct radae_config radae_config_t;

typedef struct {
    int sample_rate;
    int num_carriers;
    int symbol_rate;
    float cp_length;
    int pilot_spacing;

    int fft_size;
    int symbol_samples;
    int cp_samples;

    complex float *fft_in;
    complex float *fft_out;
    complex float pilots[MAX_CARRIERS];
} ofdm_modem_t;

/**
 * Initialize OFDM modem
 * @param ofdm OFDM modem structure
 * @param config Configuration
 * @return 0 on success, -1 on error
 */
int ofdm_init(ofdm_modem_t *ofdm, radae_config_t *config);

/**
 * Modulate data to OFDM signal
 * @param ofdm OFDM modem
 * @param data Input data (latent vector)
 * @param data_len Data length
 * @param output Output IQ samples
 * @return Number of samples generated
 */
int ofdm_modulate(ofdm_modem_t *ofdm, const float *data, int data_len,
                  complex float *output);

/**
 * Demodulate OFDM signal to data
 * @param ofdm OFDM modem
 * @param input Input IQ samples
 * @param data Output data (latent vector)
 * @param data_len Expected data length
 * @return 0 on success, -1 on error
 */
int ofdm_demodulate(ofdm_modem_t *ofdm, const complex float *input,
                    float *data, int data_len);

/**
 * Cleanup OFDM modem
 * @param ofdm OFDM modem
 */
void ofdm_cleanup(ofdm_modem_t *ofdm);

#endif /* OFDM_NEON_H */
