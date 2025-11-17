#include "radae_npu.h"
#include "npu_wrapper.h"

#include <cstring>
#include <cmath>
#include <algorithm>
#include <vector>
#include <memory>
#include <arm_neon.h>

namespace radae {

// Decoder implementation
class DecoderImpl : public NPUWrapper {
public:
    DecoderImpl() : input_buffer_(RADAE_FEATURE_DIM),
                    output_buffer_(RADAE_FRAME_SIZE),
                    postprocessed_buffer_(RADAE_FRAME_SIZE) {}

    bool decode(const float* features_in, int16_t* audio_out) {
        auto start = std::chrono::high_resolution_clock::now();

        // Copy to input tensor
        float* input_tensor = interpreter_->typed_input_tensor<float>(0);
        std::memcpy(input_tensor, features_in, RADAE_FEATURE_DIM * sizeof(float));

        // Run inference
        if (interpreter_->Invoke() != kTfLiteOk) {
            return false;
        }

        // Copy from output tensor
        const float* output_tensor = interpreter_->typed_output_tensor<float>(0);
        std::memcpy(postprocessed_buffer_.data(), output_tensor,
                   RADAE_FRAME_SIZE * sizeof(float));

        // Postprocessing with NEON
        NEONPreprocessor::deEmphasis(postprocessed_buffer_.data(), RADAE_FRAME_SIZE);
        NEONPreprocessor::floatToInt16(postprocessed_buffer_.data(), audio_out,
                                       RADAE_FRAME_SIZE);

        // Update profiling
        auto end = std::chrono::high_resolution_clock::now();
        last_profile_.inference_time =
            std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        last_profile_.used_npu = using_npu_;
        last_profile_.input_bytes = RADAE_FEATURE_DIM * sizeof(float);
        last_profile_.output_bytes = RADAE_FRAME_SIZE * sizeof(float);

        updateStats();

        return true;
    }

    bool decodeBatch(const float* features_in, int16_t* audio_out, int batch_size) {
        for (int i = 0; i < batch_size; i++) {
            if (!decode(features_in + i * RADAE_FEATURE_DIM,
                       audio_out + i * RADAE_FRAME_SIZE)) {
                return false;
            }
        }
        return true;
    }

    void reset() {
        std::fill(input_buffer_.begin(), input_buffer_.end(), 0.0f);
        std::fill(output_buffer_.begin(), output_buffer_.end(), 0);
        std::fill(postprocessed_buffer_.begin(), postprocessed_buffer_.end(), 0.0f);
        stats_ = {};
    }

    const radae_stats_t& getStats() const { return stats_; }

private:
    std::vector<float> input_buffer_;
    std::vector<int16_t> output_buffer_;
    std::vector<float> postprocessed_buffer_;
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
struct radae_decoder_s {
    std::unique_ptr<radae::DecoderImpl> impl;
};

extern "C" {

radae_decoder_t radae_decoder_create(const radae_config_t* config, radae_error_t* error) {
    if (!config || !config->model_path) {
        if (error) *error = RADAE_ERROR_INVALID_PARAM;
        return nullptr;
    }

    auto decoder = new radae_decoder_s();
    decoder->impl = std::make_unique<radae::DecoderImpl>();

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
    if (!decoder->impl->initialize(config->model_path, npu_config)) {
        delete decoder;
        if (error) *error = RADAE_ERROR_MODEL_LOAD;
        return nullptr;
    }

    if (error) *error = RADAE_OK;
    return decoder;
}

void radae_decoder_destroy(radae_decoder_t decoder) {
    if (decoder) {
        delete decoder;
    }
}

radae_error_t radae_decoder_process(
    radae_decoder_t decoder,
    const float* features_in,
    int16_t* audio_out
) {
    if (!decoder || !features_in || !audio_out) {
        return RADAE_ERROR_INVALID_PARAM;
    }

    if (!decoder->impl->decode(features_in, audio_out)) {
        return RADAE_ERROR_INFERENCE;
    }

    return RADAE_OK;
}

radae_error_t radae_decoder_process_batch(
    radae_decoder_t decoder,
    const float* features_in,
    int16_t* audio_out,
    int batch_size
) {
    if (!decoder || !features_in || !audio_out || batch_size <= 0) {
        return RADAE_ERROR_INVALID_PARAM;
    }

    if (!decoder->impl->decodeBatch(features_in, audio_out, batch_size)) {
        return RADAE_ERROR_INFERENCE;
    }

    return RADAE_OK;
}

void radae_decoder_reset(radae_decoder_t decoder) {
    if (decoder) {
        decoder->impl->reset();
    }
}

radae_error_t radae_decoder_get_stats(radae_decoder_t decoder, radae_stats_t* stats) {
    if (!decoder || !stats) {
        return RADAE_ERROR_INVALID_PARAM;
    }

    *stats = decoder->impl->getStats();
    return RADAE_OK;
}

float radae_compute_mse(const float* a, const float* b, size_t len) {
    if (!a || !b || len == 0) return -1.0f;

    float sum = 0.0f;
    for (size_t i = 0; i < len; i++) {
        float diff = a[i] - b[i];
        sum += diff * diff;
    }
    return sum / len;
}

float radae_compute_snr(const int16_t* signal, const int16_t* noise, size_t len) {
    if (!signal || !noise || len == 0) return -1.0f;

    float signal_power = 0.0f;
    float noise_power = 0.0f;

    for (size_t i = 0; i < len; i++) {
        float s = signal[i] / 32768.0f;
        float n = noise[i] / 32768.0f;
        signal_power += s * s;
        noise_power += n * n;
    }

    if (noise_power < 1e-10f) return 100.0f; // Very high SNR

    return 10.0f * log10f(signal_power / noise_power);
}

} // extern "C"
