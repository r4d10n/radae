/*
 * Configuration File Parser for RADAE
 *
 * Parses INI-style configuration files
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "config_parser.h"

#define MAX_LINE 256

// Set default configuration values
void config_set_defaults(radae_config_t *config) {
    // Audio defaults
    config->audio_sample_rate = 16000;
    config->audio_channels = 1;
    config->audio_period_size = 160;
    config->audio_buffer_size = 1600;
    strncpy(config->audio_capture_device, "hw:0,0", sizeof(config->audio_capture_device));
    strncpy(config->audio_playback_device, "hw:0,0", sizeof(config->audio_playback_device));

    // OFDM defaults
    config->ofdm_sample_rate = 8000;
    config->ofdm_num_carriers = 30;
    config->ofdm_symbol_rate = 50;
    config->ofdm_cp_length = 0.002;
    config->ofdm_pilot_spacing = 4;

    // NPU defaults
    config->npu_enable = 1;
    strncpy(config->npu_encoder_model, "/usr/share/radae/radae_encoder_int8.tflite",
            sizeof(config->npu_encoder_model));
    strncpy(config->npu_decoder_model, "/usr/share/radae/radae_decoder_int8.tflite",
            sizeof(config->npu_decoder_model));
    config->npu_num_threads = 2;

    // PTT defaults
    config->ptt_enable = 0;
    strncpy(config->ptt_gpio_chip, "gpiochip0", sizeof(config->ptt_gpio_chip));
    config->ptt_gpio_line = 12;
    config->ptt_active_low = 0;
    config->ptt_delay_ms = 100;

    // Logging defaults
    strncpy(config->log_level, "info", sizeof(config->log_level));
    strncpy(config->log_file, "/var/log/radae/radae.log", sizeof(config->log_file));
}

// Trim whitespace from string
static char *trim(char *str) {
    char *end;

    // Trim leading space
    while (isspace((unsigned char)*str)) str++;

    if (*str == 0) return str;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;

    end[1] = '\0';
    return str;
}

// Parse key=value pair
static int parse_keyvalue(radae_config_t *config, const char *section,
                         const char *key, const char *value) {
    if (strcmp(section, "audio") == 0) {
        if (strcmp(key, "sample_rate") == 0) {
            config->audio_sample_rate = atoi(value);
        } else if (strcmp(key, "channels") == 0) {
            config->audio_channels = atoi(value);
        } else if (strcmp(key, "period_size") == 0) {
            config->audio_period_size = atoi(value);
        } else if (strcmp(key, "buffer_size") == 0) {
            config->audio_buffer_size = atoi(value);
        } else if (strcmp(key, "capture_device") == 0) {
            strncpy(config->audio_capture_device, value,
                    sizeof(config->audio_capture_device) - 1);
        } else if (strcmp(key, "playback_device") == 0) {
            strncpy(config->audio_playback_device, value,
                    sizeof(config->audio_playback_device) - 1);
        }
    } else if (strcmp(section, "ofdm") == 0) {
        if (strcmp(key, "sample_rate") == 0) {
            config->ofdm_sample_rate = atoi(value);
        } else if (strcmp(key, "num_carriers") == 0) {
            config->ofdm_num_carriers = atoi(value);
        } else if (strcmp(key, "symbol_rate") == 0) {
            config->ofdm_symbol_rate = atoi(value);
        } else if (strcmp(key, "cp_length") == 0) {
            config->ofdm_cp_length = atof(value);
        } else if (strcmp(key, "pilot_spacing") == 0) {
            config->ofdm_pilot_spacing = atoi(value);
        }
    } else if (strcmp(section, "npu") == 0) {
        if (strcmp(key, "enable") == 0) {
            config->npu_enable = atoi(value);
        } else if (strcmp(key, "encoder_model") == 0) {
            strncpy(config->npu_encoder_model, value,
                    sizeof(config->npu_encoder_model) - 1);
        } else if (strcmp(key, "decoder_model") == 0) {
            strncpy(config->npu_decoder_model, value,
                    sizeof(config->npu_decoder_model) - 1);
        } else if (strcmp(key, "num_threads") == 0) {
            config->npu_num_threads = atoi(value);
        }
    } else if (strcmp(section, "ptt") == 0) {
        if (strcmp(key, "enable") == 0) {
            config->ptt_enable = atoi(value);
        } else if (strcmp(key, "gpio_chip") == 0) {
            strncpy(config->ptt_gpio_chip, value,
                    sizeof(config->ptt_gpio_chip) - 1);
        } else if (strcmp(key, "gpio_line") == 0) {
            config->ptt_gpio_line = atoi(value);
        } else if (strcmp(key, "active_low") == 0) {
            config->ptt_active_low = atoi(value);
        } else if (strcmp(key, "delay_ms") == 0) {
            config->ptt_delay_ms = atoi(value);
        }
    } else if (strcmp(section, "logging") == 0) {
        if (strcmp(key, "level") == 0) {
            strncpy(config->log_level, value, sizeof(config->log_level) - 1);
        } else if (strcmp(key, "file") == 0) {
            strncpy(config->log_file, value, sizeof(config->log_file) - 1);
        }
    }

    return 0;
}

int config_load(const char *filename, radae_config_t *config) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        return -1;
    }

    config_set_defaults(config);

    char line[MAX_LINE];
    char section[64] = "";

    while (fgets(line, sizeof(line), fp)) {
        char *p = trim(line);

        // Skip comments and empty lines
        if (*p == '#' || *p == ';' || *p == '\0') {
            continue;
        }

        // Section header [section]
        if (*p == '[') {
            char *end = strchr(p, ']');
            if (end) {
                *end = '\0';
                strncpy(section, p + 1, sizeof(section) - 1);
            }
            continue;
        }

        // Key = value
        char *eq = strchr(p, '=');
        if (eq) {
            *eq = '\0';
            char *key = trim(p);
            char *value = trim(eq + 1);

            // Remove quotes from value
            if (*value == '"') {
                value++;
                char *end = strchr(value, '"');
                if (end) *end = '\0';
            }

            parse_keyvalue(config, section, key, value);
        }
    }

    fclose(fp);
    return 0;
}

void config_print(radae_config_t *config) {
    printf("\nConfiguration:\n");
    printf("  [audio]\n");
    printf("    sample_rate = %d\n", config->audio_sample_rate);
    printf("    channels = %d\n", config->audio_channels);
    printf("    period_size = %d\n", config->audio_period_size);
    printf("    buffer_size = %d\n", config->audio_buffer_size);
    printf("    capture_device = %s\n", config->audio_capture_device);
    printf("    playback_device = %s\n", config->audio_playback_device);

    printf("  [ofdm]\n");
    printf("    sample_rate = %d\n", config->ofdm_sample_rate);
    printf("    num_carriers = %d\n", config->ofdm_num_carriers);
    printf("    symbol_rate = %d\n", config->ofdm_symbol_rate);
    printf("    cp_length = %.3f\n", config->ofdm_cp_length);
    printf("    pilot_spacing = %d\n", config->ofdm_pilot_spacing);

    printf("  [npu]\n");
    printf("    enable = %d\n", config->npu_enable);
    printf("    encoder_model = %s\n", config->npu_encoder_model);
    printf("    decoder_model = %s\n", config->npu_decoder_model);
    printf("    num_threads = %d\n", config->npu_num_threads);

    printf("  [ptt]\n");
    printf("    enable = %d\n", config->ptt_enable);
    printf("    gpio_chip = %s\n", config->ptt_gpio_chip);
    printf("    gpio_line = %d\n", config->ptt_gpio_line);
    printf("    active_low = %d\n", config->ptt_active_low);
    printf("    delay_ms = %d\n", config->ptt_delay_ms);

    printf("  [logging]\n");
    printf("    level = %s\n", config->log_level);
    printf("    file = %s\n", config->log_file);
    printf("\n");
}
