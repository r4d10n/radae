/*
 * Configuration Parser Header
 */

#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#define FEATURE_SIZE 20

typedef struct radae_config {
    // Audio configuration
    int audio_sample_rate;
    int audio_channels;
    int audio_period_size;
    int audio_buffer_size;
    char audio_capture_device[64];
    char audio_playback_device[64];

    // OFDM configuration
    int ofdm_sample_rate;
    int ofdm_num_carriers;
    int ofdm_symbol_rate;
    float ofdm_cp_length;
    int ofdm_pilot_spacing;

    // NPU configuration
    int npu_enable;
    char npu_encoder_model[256];
    char npu_decoder_model[256];
    int npu_num_threads;

    // PTT configuration
    int ptt_enable;
    char ptt_gpio_chip[32];
    int ptt_gpio_line;
    int ptt_active_low;
    int ptt_delay_ms;

    // Logging configuration
    char log_level[16];
    char log_file[256];
} radae_config_t;

/**
 * Set default configuration values
 * @param config Configuration structure
 */
void config_set_defaults(radae_config_t *config);

/**
 * Load configuration from file
 * @param filename Configuration file path
 * @param config Configuration structure
 * @return 0 on success, -1 on error
 */
int config_load(const char *filename, radae_config_t *config);

/**
 * Print configuration
 * @param config Configuration structure
 */
void config_print(radae_config_t *config);

#endif /* CONFIG_PARSER_H */
