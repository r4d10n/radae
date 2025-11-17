/*
 * RADAE Full-Duplex HF Voice Application
 * Main entry point for TX/RX operation with NPU acceleration
 *
 * Copyright (c) 2025 RADAE Project
 * Licensed under MIT License
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <getopt.h>

#include "audio_capture.h"
#include "config_parser.h"
#include "ptt_control.h"
#include "ofdm_neon.h"

#ifdef ENABLE_NPU
#include <radae/radae_npu.h>
#endif

#define VERSION "1.0.0"

// Global state
static volatile int g_running = 1;
static radae_config_t g_config;
static audio_interface_t g_audio;
static ptt_control_t g_ptt;
static ofdm_modem_t g_ofdm_tx, g_ofdm_rx;

#ifdef ENABLE_NPU
static npu_encoder_t *g_encoder = NULL;
static npu_decoder_t *g_decoder = NULL;
#endif

// Signal handler for clean shutdown
void signal_handler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        fprintf(stderr, "\nShutdown signal received, exiting...\n");
        g_running = 0;
    }
}

// TX thread - capture audio, encode, modulate, transmit
void *tx_thread(void *arg) {
    int16_t audio_buffer[AUDIO_PERIOD_SIZE];
    float features[FEATURE_SIZE];
    float feature_buffer[4][FEATURE_SIZE];
    float latent[LATENT_SIZE];
    complex float iq_samples[OFDM_SAMPLES];
    int frame_count = 0;

    fprintf(stderr, "TX thread started\n");

    while (g_running) {
        // Check PTT state
        if (g_config.ptt_enable && !ptt_is_active(&g_ptt)) {
            usleep(10000);  // Sleep 10ms when not transmitting
            continue;
        }

        // Capture audio (10ms frame @ 16kHz = 160 samples)
        if (audio_capture(&g_audio, audio_buffer, AUDIO_PERIOD_SIZE) < 0) {
            continue;
        }

        // Extract features (simplified - would use FARGAN in production)
        // For now, basic feature extraction
        for (int i = 0; i < FEATURE_SIZE && i < AUDIO_PERIOD_SIZE; i++) {
            features[i] = audio_buffer[i] / 32768.0f;
        }

        // Buffer 4 frames (40ms total)
        memcpy(feature_buffer[frame_count % 4], features, FEATURE_SIZE * sizeof(float));
        frame_count++;

        if (frame_count % 4 == 0) {
#ifdef ENABLE_NPU
            // Encode with NPU (4 frames → 80 latent)
            if (g_encoder) {
                npu_encode(g_encoder, &feature_buffer[0][0], latent);
            }
#else
            // CPU fallback encoding would go here
            memset(latent, 0, LATENT_SIZE * sizeof(float));
#endif

            // OFDM modulate
            ofdm_modulate(&g_ofdm_tx, latent, LATENT_SIZE, iq_samples);

            // Send to RF interface (placeholder - would use SDR)
            // sdr_transmit(iq_samples, OFDM_SAMPLES);
        }
    }

    fprintf(stderr, "TX thread stopped\n");
    return NULL;
}

// RX thread - receive, demodulate, decode, play audio
void *rx_thread(void *arg) {
    complex float iq_samples[OFDM_SAMPLES];
    float latent[LATENT_SIZE];
    float features[4][FEATURE_SIZE + 1];  // Decoder outputs 21 features
    int16_t audio_buffer[AUDIO_PERIOD_SIZE];

    fprintf(stderr, "RX thread started\n");

    while (g_running) {
        // Receive from RF interface (placeholder)
        // if (sdr_receive(iq_samples, OFDM_SAMPLES) < 0) continue;

        // For testing, generate silence
        memset(iq_samples, 0, OFDM_SAMPLES * sizeof(complex float));
        usleep(40000);  // 40ms delay

        // OFDM demodulate
        if (ofdm_demodulate(&g_ofdm_rx, iq_samples, latent, LATENT_SIZE) < 0) {
            continue;
        }

#ifdef ENABLE_NPU
        // Decode with NPU (80 latent → 4 frames × 21 features)
        if (g_decoder) {
            npu_decode(g_decoder, latent, &features[0][0]);
        }
#else
        // CPU fallback decoding
        memset(features, 0, 4 * (FEATURE_SIZE + 1) * sizeof(float));
#endif

        // Synthesize audio from features (simplified - would use FARGAN)
        // For now, basic synthesis
        for (int frame = 0; frame < 4; frame++) {
            for (int i = 0; i < AUDIO_PERIOD_SIZE; i++) {
                float sample = (i < FEATURE_SIZE) ? features[frame][i] : 0.0f;
                audio_buffer[i] = (int16_t)(sample * 32768.0f);
            }

            // Play audio
            audio_playback(&g_audio, audio_buffer, AUDIO_PERIOD_SIZE);
        }
    }

    fprintf(stderr, "RX thread stopped\n");
    return NULL;
}

void print_usage(const char *prog) {
    printf("RADAE Full-Duplex HF Voice Application v%s\n", VERSION);
    printf("Usage: %s [OPTIONS]\n\n", prog);
    printf("Options:\n");
    printf("  -c, --config FILE     Configuration file (default: /etc/radae/radae.conf)\n");
    printf("  -t, --tx-only         Transmit only mode\n");
    printf("  -r, --rx-only         Receive only mode\n");
    printf("  -d, --device NAME     Audio device name\n");
    printf("  -v, --verbose         Verbose output\n");
    printf("  -h, --help            Show this help\n");
    printf("  -V, --version         Show version\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s                    Full-duplex operation\n", prog);
    printf("  %s -t                 Transmit only\n", prog);
    printf("  %s -c custom.conf     Use custom configuration\n", prog);
    printf("\n");
}

int main(int argc, char **argv) {
    const char *config_file = "/etc/radae/radae.conf";
    int tx_only = 0, rx_only = 0;
    int verbose = 0;
    pthread_t tx_tid, rx_tid;

    // Parse command line options
    static struct option long_options[] = {
        {"config",  required_argument, 0, 'c'},
        {"tx-only", no_argument,       0, 't'},
        {"rx-only", no_argument,       0, 'r'},
        {"device",  required_argument, 0, 'd'},
        {"verbose", no_argument,       0, 'v'},
        {"help",    no_argument,       0, 'h'},
        {"version", no_argument,       0, 'V'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "c:trd:vhV", long_options, NULL)) != -1) {
        switch (opt) {
            case 'c':
                config_file = optarg;
                break;
            case 't':
                tx_only = 1;
                break;
            case 'r':
                rx_only = 1;
                break;
            case 'd':
                // Override audio device
                break;
            case 'v':
                verbose = 1;
                break;
            case 'V':
                printf("RADAE v%s\n", VERSION);
                return 0;
            case 'h':
            default:
                print_usage(argv[0]);
                return (opt == 'h') ? 0 : 1;
        }
    }

    printf("RADAE Full-Duplex HF Voice Application v%s\n", VERSION);
    printf("Loading configuration from: %s\n", config_file);

    // Load configuration
    if (config_load(config_file, &g_config) < 0) {
        fprintf(stderr, "Failed to load configuration, using defaults\n");
        config_set_defaults(&g_config);
    }

    if (verbose) {
        config_print(&g_config);
    }

    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Initialize audio interface
    printf("Initializing audio interface...\n");
    if (audio_init(&g_audio, &g_config) < 0) {
        fprintf(stderr, "Failed to initialize audio\n");
        return 1;
    }

    // Initialize PTT control
    if (g_config.ptt_enable) {
        printf("Initializing PTT control (GPIO %s:%d)...\n",
               g_config.ptt_gpio_chip, g_config.ptt_gpio_line);
        if (ptt_init(&g_ptt, &g_config) < 0) {
            fprintf(stderr, "Warning: Failed to initialize PTT\n");
            g_config.ptt_enable = 0;
        }
    }

    // Initialize OFDM modems
    printf("Initializing OFDM modem...\n");
    ofdm_init(&g_ofdm_tx, &g_config);
    ofdm_init(&g_ofdm_rx, &g_config);

#ifdef ENABLE_NPU
    // Initialize NPU encoder/decoder
    if (g_config.npu_enable) {
        printf("Initializing NPU encoder...\n");
        g_encoder = npu_encoder_create(g_config.npu_encoder_model);
        if (!g_encoder) {
            fprintf(stderr, "Warning: Failed to create NPU encoder, using CPU\n");
        }

        printf("Initializing NPU decoder...\n");
        g_decoder = npu_decoder_create(g_config.npu_decoder_model);
        if (!g_decoder) {
            fprintf(stderr, "Warning: Failed to create NPU decoder, using CPU\n");
        }
    }
#endif

    // Start TX/RX threads
    printf("Starting %s operation...\n",
           tx_only ? "TX" : (rx_only ? "RX" : "full-duplex"));

    if (!rx_only) {
        pthread_create(&tx_tid, NULL, tx_thread, NULL);
    }

    if (!tx_only) {
        pthread_create(&rx_tid, NULL, rx_thread, NULL);
    }

    // Main loop - just wait for shutdown signal
    while (g_running) {
        sleep(1);
    }

    // Cleanup
    printf("Shutting down...\n");

    if (!rx_only) {
        pthread_join(tx_tid, NULL);
    }
    if (!tx_only) {
        pthread_join(rx_tid, NULL);
    }

#ifdef ENABLE_NPU
    if (g_encoder) npu_encoder_destroy(g_encoder);
    if (g_decoder) npu_decoder_destroy(g_decoder);
#endif

    ofdm_cleanup(&g_ofdm_tx);
    ofdm_cleanup(&g_ofdm_rx);

    if (g_config.ptt_enable) {
        ptt_cleanup(&g_ptt);
    }

    audio_cleanup(&g_audio);

    printf("Shutdown complete\n");
    return 0;
}
