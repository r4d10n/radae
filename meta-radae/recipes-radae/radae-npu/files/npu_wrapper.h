#ifndef NPU_WRAPPER_H
#define NPU_WRAPPER_H

#include <tensorflow/lite/interpreter.h>
#include <tensorflow/lite/kernels/register.h>
#include <tensorflow/lite/model.h>
#include <tensorflow/lite/optional_debug_tools.h>

#include <memory>
#include <string>
#include <vector>
#include <chrono>

namespace radae {

// NPU delegate configuration
struct NPUDelegateConfig {
    bool enable_npu = true;
    bool enable_neon = true;
    bool allow_cpu_fallback = true;
    int num_threads = 4;
    int cache_size_kb = 4096;

    enum QuantizationMode {
        NONE = 0,
        DYNAMIC = 1,
        INT8 = 2,
        INT16 = 3
    } quantization = INT8;
};

// Performance profiling
struct InferenceProfile {
    std::chrono::microseconds inference_time{0};
    size_t input_bytes = 0;
    size_t output_bytes = 0;
    bool used_npu = false;

    void reset() {
        inference_time = std::chrono::microseconds{0};
        input_bytes = 0;
        output_bytes = 0;
        used_npu = false;
    }
};

// Base NPU wrapper class
class NPUWrapper {
public:
    NPUWrapper() = default;
    virtual ~NPUWrapper() = default;

    // Initialize with model file
    bool initialize(const std::string& model_path, const NPUDelegateConfig& config);

    // Check if NPU is available and enabled
    bool isNPUAvailable() const { return npu_available_; }
    bool isUsingNPU() const { return using_npu_; }

    // Get model information
    std::vector<int> getInputShape() const;
    std::vector<int> getOutputShape() const;
    size_t getInputSize() const;
    size_t getOutputSize() const;

    // Profiling
    const InferenceProfile& getLastProfile() const { return last_profile_; }
    void resetProfiling();

protected:
    // TensorFlow Lite components
    std::unique_ptr<tflite::FlatBufferModel> model_;
    std::unique_ptr<tflite::Interpreter> interpreter_;
    tflite::ops::builtin::BuiltinOpResolver resolver_;

    // NPU delegate
    void* npu_delegate_ = nullptr;

    // Configuration
    NPUDelegateConfig config_;
    bool npu_available_ = false;
    bool using_npu_ = false;

    // Profiling
    InferenceProfile last_profile_;

    // Helper methods
    bool loadModel(const std::string& model_path);
    bool setupInterpreter();
    bool attachNPUDelegate();
    bool allocateTensors();
    void detectNPUAvailability();

    // Template for running inference
    template<typename InputType, typename OutputType>
    bool runInference(const InputType* input, OutputType* output, size_t input_size, size_t output_size);
};

// NEON-optimized preprocessing
class NEONPreprocessor {
public:
    // Convert int16 PCM to float32 normalized [-1, 1]
    static void int16ToFloat(const int16_t* input, float* output, size_t length);

    // Convert float32 to int16 PCM with clipping
    static void floatToInt16(const float* input, int16_t* output, size_t length);

    // Apply pre-emphasis filter
    static void preEmphasis(float* data, size_t length, float coef = 0.97f);

    // Remove pre-emphasis
    static void deEmphasis(float* data, size_t length, float coef = 0.97f);

    // Compute RMS
    static float computeRMS(const float* data, size_t length);

    // Apply gain
    static void applyGain(float* data, size_t length, float gain);
};

// Streaming buffer for frame-based processing
template<typename T>
class StreamingBuffer {
public:
    StreamingBuffer(size_t frame_size, size_t overlap = 0)
        : frame_size_(frame_size), overlap_(overlap), buffer_(frame_size + overlap, 0) {}

    // Add samples and check if frame is ready
    bool addSamples(const T* samples, size_t count);

    // Get current frame
    const T* getFrame() const { return buffer_.data(); }

    // Reset buffer
    void reset() {
        std::fill(buffer_.begin(), buffer_.end(), 0);
        write_pos_ = 0;
    }

private:
    size_t frame_size_;
    size_t overlap_;
    std::vector<T> buffer_;
    size_t write_pos_ = 0;
};

// Quantization utilities
class QuantizationUtils {
public:
    // Quantize float32 to int8
    static void quantizeInt8(const float* input, int8_t* output, size_t length,
                            float scale, int32_t zero_point);

    // Dequantize int8 to float32
    static void dequantizeInt8(const int8_t* input, float* output, size_t length,
                              float scale, int32_t zero_point);

    // Compute quantization parameters
    static void computeQuantParams(const float* data, size_t length,
                                   float& scale, int32_t& zero_point);
};

} // namespace radae

#endif // NPU_WRAPPER_H
