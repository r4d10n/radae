#include "npu_wrapper.h"

#include <tensorflow/lite/delegates/external/external_delegate.h>
#include <arm_neon.h>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>

namespace radae {

// NPUWrapper implementation
bool NPUWrapper::initialize(const std::string& model_path, const NPUDelegateConfig& config) {
    config_ = config;

    // Detect NPU availability
    detectNPUAvailability();

    // Load model
    if (!loadModel(model_path)) {
        std::cerr << "Failed to load model: " << model_path << std::endl;
        return false;
    }

    // Setup interpreter
    if (!setupInterpreter()) {
        std::cerr << "Failed to setup interpreter" << std::endl;
        return false;
    }

    // Attach NPU delegate if available and enabled
    if (config_.enable_npu && npu_available_) {
        if (!attachNPUDelegate()) {
            std::cerr << "Failed to attach NPU delegate";
            if (config_.allow_cpu_fallback) {
                std::cerr << ", falling back to CPU" << std::endl;
                using_npu_ = false;
            } else {
                std::cerr << std::endl;
                return false;
            }
        } else {
            using_npu_ = true;
        }
    }

    // Allocate tensors
    if (!allocateTensors()) {
        std::cerr << "Failed to allocate tensors" << std::endl;
        return false;
    }

    return true;
}

void NPUWrapper::detectNPUAvailability() {
    // Check for NPU device node
    std::ifstream npu_device("/dev/galcore");
    npu_available_ = npu_device.good();

    // Also check for VeriSilicon NPU library
    if (npu_available_) {
        // Try to dlopen the VSI NPU delegate library
        void* handle = dlopen("libvsi_npu_delegate.so", RTLD_LAZY);
        if (!handle) {
            npu_available_ = false;
        } else {
            dlclose(handle);
        }
    }
}

bool NPUWrapper::loadModel(const std::string& model_path) {
    model_ = tflite::FlatBufferModel::BuildFromFile(model_path.c_str());
    return model_ != nullptr;
}

bool NPUWrapper::setupInterpreter() {
    tflite::InterpreterBuilder builder(*model_, resolver_);
    builder(&interpreter_);

    if (!interpreter_) {
        return false;
    }

    // Set number of threads
    interpreter_->SetNumThreads(config_.num_threads);

    // Allow fp16 precision for better NPU performance
    interpreter_->SetAllowFp16PrecisionForFp32(true);

    return true;
}

bool NPUWrapper::attachNPUDelegate() {
    // Load VeriSilicon NPU delegate
    TfLiteExternalDelegateOptions options = TfLiteExternalDelegateOptionsDefault(
        "libvsi_npu_delegate.so");

    // Set NPU-specific options
    std::vector<std::string> option_keys = {
        "cache_file_path",
        "allowed_cache_mode",
        "allowed_builtin_code"
    };

    std::vector<std::string> option_values = {
        "/tmp/npu_cache",
        "1",  // Enable caching
        "0"   // Allow all ops
    };

    options.count = option_keys.size();
    std::vector<const char*> keys;
    std::vector<const char*> values;
    for (const auto& k : option_keys) keys.push_back(k.c_str());
    for (const auto& v : option_values) values.push_back(v.c_str());
    options.keys = keys.data();
    options.values = values.data();

    npu_delegate_ = TfLiteExternalDelegateCreate(&options);

    if (!npu_delegate_) {
        return false;
    }

    if (interpreter_->ModifyGraphWithDelegate(
            reinterpret_cast<TfLiteDelegate*>(npu_delegate_)) != kTfLiteOk) {
        TfLiteExternalDelegateDelete(npu_delegate_);
        npu_delegate_ = nullptr;
        return false;
    }

    return true;
}

bool NPUWrapper::allocateTensors() {
    return interpreter_->AllocateTensors() == kTfLiteOk;
}

std::vector<int> NPUWrapper::getInputShape() const {
    if (!interpreter_) return {};
    int input_idx = interpreter_->inputs()[0];
    TfLiteIntArray* dims = interpreter_->tensor(input_idx)->dims;
    return std::vector<int>(dims->data, dims->data + dims->size);
}

std::vector<int> NPUWrapper::getOutputShape() const {
    if (!interpreter_) return {};
    int output_idx = interpreter_->outputs()[0];
    TfLiteIntArray* dims = interpreter_->tensor(output_idx)->dims;
    return std::vector<int>(dims->data, dims->data + dims->size);
}

size_t NPUWrapper::getInputSize() const {
    auto shape = getInputShape();
    size_t size = 1;
    for (int dim : shape) size *= dim;
    return size;
}

size_t NPUWrapper::getOutputSize() const {
    auto shape = getOutputShape();
    size_t size = 1;
    for (int dim : shape) size *= dim;
    return size;
}

void NPUWrapper::resetProfiling() {
    last_profile_.reset();
}

// NEONPreprocessor implementation
void NEONPreprocessor::int16ToFloat(const int16_t* input, float* output, size_t length) {
#ifdef __ARM_NEON
    const float scale = 1.0f / 32768.0f;
    float32x4_t scale_vec = vdupq_n_f32(scale);

    size_t i = 0;
    // Process 8 samples at a time
    for (; i + 8 <= length; i += 8) {
        // Load 8 int16 values
        int16x8_t input_vec = vld1q_s16(input + i);

        // Convert to two int32x4 vectors
        int32x4_t int32_low = vmovl_s16(vget_low_s16(input_vec));
        int32x4_t int32_high = vmovl_s16(vget_high_s16(input_vec));

        // Convert to float and scale
        float32x4_t float_low = vmulq_f32(vcvtq_f32_s32(int32_low), scale_vec);
        float32x4_t float_high = vmulq_f32(vcvtq_f32_s32(int32_high), scale_vec);

        // Store results
        vst1q_f32(output + i, float_low);
        vst1q_f32(output + i + 4, float_high);
    }

    // Process remaining samples
    for (; i < length; i++) {
        output[i] = input[i] * scale;
    }
#else
    const float scale = 1.0f / 32768.0f;
    for (size_t i = 0; i < length; i++) {
        output[i] = input[i] * scale;
    }
#endif
}

void NEONPreprocessor::floatToInt16(const float* input, int16_t* output, size_t length) {
#ifdef __ARM_NEON
    const float scale = 32767.0f;
    float32x4_t scale_vec = vdupq_n_f32(scale);
    float32x4_t min_vec = vdupq_n_f32(-32768.0f);
    float32x4_t max_vec = vdupq_n_f32(32767.0f);

    size_t i = 0;
    // Process 8 samples at a time
    for (; i + 8 <= length; i += 8) {
        // Load and scale
        float32x4_t float_low = vld1q_f32(input + i);
        float32x4_t float_high = vld1q_f32(input + i + 4);

        float_low = vmulq_f32(float_low, scale_vec);
        float_high = vmulq_f32(float_high, scale_vec);

        // Clamp
        float_low = vmaxq_f32(vminq_f32(float_low, max_vec), min_vec);
        float_high = vmaxq_f32(vminq_f32(float_high, max_vec), min_vec);

        // Convert to int32
        int32x4_t int32_low = vcvtq_s32_f32(float_low);
        int32x4_t int32_high = vcvtq_s32_f32(float_high);

        // Narrow to int16 and store
        int16x4_t int16_low = vmovn_s32(int32_low);
        int16x4_t int16_high = vmovn_s32(int32_high);
        int16x8_t result = vcombine_s16(int16_low, int16_high);

        vst1q_s16(output + i, result);
    }

    // Process remaining samples
    for (; i < length; i++) {
        float val = input[i] * scale;
        val = std::max(-32768.0f, std::min(32767.0f, val));
        output[i] = static_cast<int16_t>(val);
    }
#else
    const float scale = 32767.0f;
    for (size_t i = 0; i < length; i++) {
        float val = input[i] * scale;
        val = std::max(-32768.0f, std::min(32767.0f, val));
        output[i] = static_cast<int16_t>(val);
    }
#endif
}

void NEONPreprocessor::preEmphasis(float* data, size_t length, float coef) {
    if (length == 0) return;

    // Process from end to beginning to avoid overwriting
    for (size_t i = length - 1; i > 0; i--) {
        data[i] -= coef * data[i - 1];
    }
}

void NEONPreprocessor::deEmphasis(float* data, size_t length, float coef) {
    if (length == 0) return;

    // Process from beginning to end
    for (size_t i = 1; i < length; i++) {
        data[i] += coef * data[i - 1];
    }
}

float NEONPreprocessor::computeRMS(const float* data, size_t length) {
#ifdef __ARM_NEON
    float32x4_t sum_vec = vdupq_n_f32(0.0f);

    size_t i = 0;
    for (; i + 4 <= length; i += 4) {
        float32x4_t val = vld1q_f32(data + i);
        sum_vec = vmlaq_f32(sum_vec, val, val);
    }

    // Sum the vector
    float sum = vgetq_lane_f32(sum_vec, 0) + vgetq_lane_f32(sum_vec, 1) +
                vgetq_lane_f32(sum_vec, 2) + vgetq_lane_f32(sum_vec, 3);

    // Process remaining
    for (; i < length; i++) {
        sum += data[i] * data[i];
    }

    return std::sqrt(sum / length);
#else
    float sum = 0.0f;
    for (size_t i = 0; i < length; i++) {
        sum += data[i] * data[i];
    }
    return std::sqrt(sum / length);
#endif
}

void NEONPreprocessor::applyGain(float* data, size_t length, float gain) {
#ifdef __ARM_NEON
    float32x4_t gain_vec = vdupq_n_f32(gain);

    size_t i = 0;
    for (; i + 4 <= length; i += 4) {
        float32x4_t val = vld1q_f32(data + i);
        val = vmulq_f32(val, gain_vec);
        vst1q_f32(data + i, val);
    }

    for (; i < length; i++) {
        data[i] *= gain;
    }
#else
    for (size_t i = 0; i < length; i++) {
        data[i] *= gain;
    }
#endif
}

// QuantizationUtils implementation
void QuantizationUtils::quantizeInt8(const float* input, int8_t* output, size_t length,
                                    float scale, int32_t zero_point) {
    for (size_t i = 0; i < length; i++) {
        int32_t val = static_cast<int32_t>(std::round(input[i] / scale)) + zero_point;
        val = std::max(-128, std::min(127, val));
        output[i] = static_cast<int8_t>(val);
    }
}

void QuantizationUtils::dequantizeInt8(const int8_t* input, float* output, size_t length,
                                      float scale, int32_t zero_point) {
    for (size_t i = 0; i < length; i++) {
        output[i] = (static_cast<int32_t>(input[i]) - zero_point) * scale;
    }
}

void QuantizationUtils::computeQuantParams(const float* data, size_t length,
                                          float& scale, int32_t& zero_point) {
    float min_val = data[0];
    float max_val = data[0];

    for (size_t i = 1; i < length; i++) {
        min_val = std::min(min_val, data[i]);
        max_val = std::max(max_val, data[i]);
    }

    // Compute scale and zero point for int8 quantization
    const float qmin = -128.0f;
    const float qmax = 127.0f;

    scale = (max_val - min_val) / (qmax - qmin);
    if (scale < 1e-10f) scale = 1e-10f;

    zero_point = static_cast<int32_t>(std::round(qmin - min_val / scale));
    zero_point = std::max(-128, std::min(127, zero_point));
}

// StreamingBuffer template instantiations
template<typename T>
bool StreamingBuffer<T>::addSamples(const T* samples, size_t count) {
    for (size_t i = 0; i < count; i++) {
        buffer_[write_pos_++] = samples[i];

        if (write_pos_ >= frame_size_) {
            // Frame is ready, shift for overlap
            if (overlap_ > 0) {
                std::memmove(buffer_.data(), buffer_.data() + frame_size_ - overlap_,
                           overlap_ * sizeof(T));
                write_pos_ = overlap_;
            } else {
                write_pos_ = 0;
            }
            return true;
        }
    }
    return false;
}

// Explicit template instantiations
template class StreamingBuffer<int16_t>;
template class StreamingBuffer<float>;

} // namespace radae
