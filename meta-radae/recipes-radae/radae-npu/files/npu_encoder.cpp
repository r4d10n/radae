#include "radae_npu.h"
#include "npu_wrapper.h"

#include <cstring>
#include <cmath>
#include <algorithm>
#include <vector>
#include <memory>
#include <arm_neon.h>

namespace radae {

// Encoder implementation
class EncoderImpl : public NPUWrapper {
public:
    EncoderImpl() : input_buffer_(RADAE_FRAME_SIZE),
                    output_buffer_(RADAE_FEATURE_DIM),
                    preprocessed_buffer_(RADAE_FRAME_SIZE) {}

    bool encode(const int16_t* audio_in, float* features_out) {
        auto start = std::chrono::high_resolution_clock::now();

        // Preprocessing with NEON
        NEONPreprocessor::int16ToFloat(audio_in, preprocessed_buffer_.data(), RADAE_FRAME_SIZE);
        NEONPreprocessor::preEmphasis(preprocessed_buffer_.data(), RADAE_FRAME_SIZE);

        // Copy to input tensor
        float* input_tensor = interpreter_->typed_input_tensor<float>(0);
        std::memcpy(input_tensor, preprocessed_buffer_.data(),
                   RADAE_FRAME_SIZE * sizeof(float));

        // Run inference
        if (interpreter_->Invoke() != kTfLiteOk) {
            return false;
        }

        // Copy from output tensor
        const float* output_tensor = interpreter_->typed_output_tensor<float>(0);
        std::memcpy(features_out, output_tensor, RADAE_FEATURE_DIM * sizeof(float));

        // Update profiling
        auto end = std::chrono::high_resolution_clock::now();
        last_profile_.inference_time =
            std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        last_profile_.used_npu = using_npu_;
        last_profile_.input_bytes = RADAE_FRAME_SIZE * sizeof(float);
        last_profile_.output_bytes = RADAE_FEATURE_DIM * sizeof(float);

        updateStats();

        return true;
    }

    bool encodeBatch(const int16_t* audio_in, float* features_out, int batch_size) {
        for (int i = 0; i < batch_size; i++) {
            if (!encode(audio_in + i * RADAE_FRAME_SIZE,
                       features_out + i * RADAE_FEATURE_DIM)) {
                return false;
            }
        }
        return true;
    }

    void reset() {
        std::fill(input_buffer_.begin(), input_buffer_.end(), 0);
        std::fill(output_buffer_.begin(), output_buffer_.end(), 0.0f);
        std::fill(preprocessed_buffer_.begin(), preprocessed_buffer_.end(), 0.0f);
        stats_ = {};
    }

    const radae_stats_t& getStats() const { return stats_; }

private:
    std::vector<int16_t> input_buffer_;
    std::vector<float> output_buffer_;
    std::vector<float> preprocessed_buffer_;
    radae_stats_t stats_ = {};

    void updateStats() {
        stats_.total_inferences++;
        if (using_npu_) {
            stats_.npu_inferences++;
        } else {
            stats_.cpu_inferences++;
        }

        float time_ms = last_profile_.inference_time.count() / 1000.0f;

        if (stats_.total_inferences == 1) {
            stats_.avg_inference_time_ms = time_ms;
            stats_.min_inference_time_ms = time_ms;
            stats_.max_inference_time_ms = time_ms;
        } else {
            stats_.avg_inference_time_ms =
                (stats_.avg_inference_time_ms * (stats_.total_inferences - 1) + time_ms)
                / stats_.total_inferences;
            stats_.min_inference_time_ms = std::min(stats_.min_inference_time_ms, time_ms);
            stats_.max_inference_time_ms = std::max(stats_.max_inference_time_ms, time_ms);
        }

        stats_.npu_utilization =
            static_cast<float>(stats_.npu_inferences) / stats_.total_inferences;
    }
};

} // namespace radae

// C API implementation
struct radae_encoder_s {
    std::unique_ptr<radae::EncoderImpl> impl;
};

extern "C" {

radae_error_t radae_get_default_config(radae_config_t* config) {
    if (!config) return RADAE_ERROR_INVALID_PARAM;

    config->quantization = RADAE_QUANT_INT8;
    config->exec_mode = RADAE_EXEC_AUTO;
    config->num_threads = 4;
    config->enable_neon = 1;
    config->cache_size_kb = 4096;
    config->enable_profiling = 0;
    config->model_path = "/usr/share/radae/models/encoder_int8.tflite";

    return RADAE_OK;
}

const char* radae_get_error_string(radae_error_t error) {
    switch (error) {
        case RADAE_OK: return "Success";
        case RADAE_ERROR_INVALID_PARAM: return "Invalid parameter";
        case RADAE_ERROR_NPU_UNAVAILABLE: return "NPU unavailable";
        case RADAE_ERROR_MODEL_LOAD: return "Failed to load model";
        case RADAE_ERROR_ALLOCATION: return "Memory allocation failed";
        case RADAE_ERROR_INFERENCE: return "Inference failed";
        case RADAE_ERROR_NOT_INITIALIZED: return "Not initialized";
        default: return "Unknown error";
    }
}

int radae_check_npu_available(void) {
    // Check for NPU device
    FILE* f = fopen("/dev/galcore", "r");
    if (f) {
        fclose(f);
        return 1;
    }
    return 0;
}

radae_encoder_t radae_encoder_create(const radae_config_t* config, radae_error_t* error) {
    if (!config || !config->model_path) {
        if (error) *error = RADAE_ERROR_INVALID_PARAM;
        return nullptr;
    }

    auto encoder = new radae_encoder_s();
    encoder->impl = std::make_unique<radae::EncoderImpl>();

    // Configure NPU delegate
    radae::NPUDelegateConfig npu_config;
    npu_config.enable_npu = (config->exec_mode != RADAE_EXEC_CPU);
    npu_config.enable_neon = config->enable_neon;
    npu_config.allow_cpu_fallback = (config->exec_mode == RADAE_EXEC_AUTO);
    npu_config.num_threads = config->num_threads;
    npu_config.cache_size_kb = config->cache_size_kb;

    switch (config->quantization) {
        case RADAE_QUANT_NONE:
            npu_config.quantization = radae::NPUDelegateConfig::NONE;
            break;
        case RADAE_QUANT_DYNAMIC:
            npu_config.quantization = radae::NPUDelegateConfig::DYNAMIC;
            break;
        case RADAE_QUANT_INT8:
            npu_config.quantization = radae::NPUDelegateConfig::INT8;
            break;
        case RADAE_QUANT_INT16:
            npu_config.quantization = radae::NPUDelegateConfig::INT16;
            break;
    }

    // Initialize model
    if (!encoder->impl->initialize(config->model_path, npu_config)) {
        delete encoder;
        if (error) *error = RADAE_ERROR_MODEL_LOAD;
        return nullptr;
    }

    if (error) *error = RADAE_OK;
    return encoder;
}

void radae_encoder_destroy(radae_encoder_t encoder) {
    if (encoder) {
        delete encoder;
    }
}

radae_error_t radae_encoder_process(
    radae_encoder_t encoder,
    const int16_t* audio_in,
    float* features_out
) {
    if (!encoder || !audio_in || !features_out) {
        return RADAE_ERROR_INVALID_PARAM;
    }

    if (!encoder->impl->encode(audio_in, features_out)) {
        return RADAE_ERROR_INFERENCE;
    }

    return RADAE_OK;
}

radae_error_t radae_encoder_process_batch(
    radae_encoder_t encoder,
    const int16_t* audio_in,
    float* features_out,
    int batch_size
) {
    if (!encoder || !audio_in || !features_out || batch_size <= 0) {
        return RADAE_ERROR_INVALID_PARAM;
    }

    if (!encoder->impl->encodeBatch(audio_in, features_out, batch_size)) {
        return RADAE_ERROR_INFERENCE;
    }

    return RADAE_OK;
}

void radae_encoder_reset(radae_encoder_t encoder) {
    if (encoder) {
        encoder->impl->reset();
    }
}

radae_error_t radae_encoder_get_stats(radae_encoder_t encoder, radae_stats_t* stats) {
    if (!encoder || !stats) {
        return RADAE_ERROR_INVALID_PARAM;
    }

    *stats = encoder->impl->getStats();
    return RADAE_OK;
}

} // extern "C"
