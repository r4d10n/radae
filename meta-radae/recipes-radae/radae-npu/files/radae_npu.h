#ifndef RADAE_NPU_H
#define RADAE_NPU_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// RADAE NPU API version
#define RADAE_NPU_VERSION_MAJOR 1
#define RADAE_NPU_VERSION_MINOR 0
#define RADAE_NPU_VERSION_PATCH 0

// Audio parameters
#define RADAE_SAMPLE_RATE 8000
#define RADAE_FRAME_SIZE 160  // 20ms at 8kHz
#define RADAE_FEATURE_DIM 20
#define RADAE_LATENT_DIM 80

// Quantization modes
typedef enum {
    RADAE_QUANT_NONE = 0,
    RADAE_QUANT_DYNAMIC = 1,
    RADAE_QUANT_INT8 = 2,
    RADAE_QUANT_INT16 = 3
} radae_quantization_t;

// Execution modes
typedef enum {
    RADAE_EXEC_NPU = 0,      // NPU only (fail if unavailable)
    RADAE_EXEC_CPU = 1,      // CPU only
    RADAE_EXEC_AUTO = 2      // Auto-select (NPU preferred)
} radae_exec_mode_t;

// Error codes
typedef enum {
    RADAE_OK = 0,
    RADAE_ERROR_INVALID_PARAM = -1,
    RADAE_ERROR_NPU_UNAVAILABLE = -2,
    RADAE_ERROR_MODEL_LOAD = -3,
    RADAE_ERROR_ALLOCATION = -4,
    RADAE_ERROR_INFERENCE = -5,
    RADAE_ERROR_NOT_INITIALIZED = -6
} radae_error_t;

// Configuration structure
typedef struct {
    radae_quantization_t quantization;
    radae_exec_mode_t exec_mode;
    int num_threads;
    int enable_neon;
    int cache_size_kb;
    int enable_profiling;
    const char* model_path;
} radae_config_t;

// Performance statistics
typedef struct {
    float avg_inference_time_ms;
    float min_inference_time_ms;
    float max_inference_time_ms;
    uint64_t total_inferences;
    uint64_t npu_inferences;
    uint64_t cpu_inferences;
    float npu_utilization;
} radae_stats_t;

// Opaque handle types
typedef struct radae_encoder_s* radae_encoder_t;
typedef struct radae_decoder_s* radae_decoder_t;

// Initialization and configuration
radae_error_t radae_get_default_config(radae_config_t* config);
const char* radae_get_error_string(radae_error_t error);
int radae_check_npu_available(void);

// Encoder API
radae_encoder_t radae_encoder_create(const radae_config_t* config, radae_error_t* error);
void radae_encoder_destroy(radae_encoder_t encoder);

radae_error_t radae_encoder_process(
    radae_encoder_t encoder,
    const int16_t* audio_in,    // [RADAE_FRAME_SIZE]
    float* features_out         // [RADAE_FEATURE_DIM]
);

radae_error_t radae_encoder_process_batch(
    radae_encoder_t encoder,
    const int16_t* audio_in,    // [batch_size * RADAE_FRAME_SIZE]
    float* features_out,        // [batch_size * RADAE_FEATURE_DIM]
    int batch_size
);

void radae_encoder_reset(radae_encoder_t encoder);
radae_error_t radae_encoder_get_stats(radae_encoder_t encoder, radae_stats_t* stats);

// Decoder API
radae_decoder_t radae_decoder_create(const radae_config_t* config, radae_error_t* error);
void radae_decoder_destroy(radae_decoder_t decoder);

radae_error_t radae_decoder_process(
    radae_decoder_t decoder,
    const float* features_in,   // [RADAE_FEATURE_DIM]
    int16_t* audio_out         // [RADAE_FRAME_SIZE]
);

radae_error_t radae_decoder_process_batch(
    radae_decoder_t decoder,
    const float* features_in,   // [batch_size * RADAE_FEATURE_DIM]
    int16_t* audio_out,        // [batch_size * RADAE_FRAME_SIZE]
    int batch_size
);

void radae_decoder_reset(radae_decoder_t decoder);
radae_error_t radae_decoder_get_stats(radae_decoder_t decoder, radae_stats_t* stats);

// Utility functions
float radae_compute_mse(const float* a, const float* b, size_t len);
float radae_compute_snr(const int16_t* signal, const int16_t* noise, size_t len);

#ifdef __cplusplus
}
#endif

#endif // RADAE_NPU_H
