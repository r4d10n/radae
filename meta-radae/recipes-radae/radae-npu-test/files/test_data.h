#ifndef TEST_DATA_H
#define TEST_DATA_H

#include <string>
#include <vector>

namespace radae {
namespace test {

// Test vector paths
struct TestVectors {
    static std::string getDataPath() {
        const char* env = getenv("RADAE_TEST_DATA");
        if (env) return std::string(env);
        return "/usr/share/radae/test-vectors";
    }

    static std::string getSpeechFile() {
        return getDataPath() + "/speech_8k.raw";
    }

    static std::string getExpectedFeaturesFile() {
        return getDataPath() + "/expected_features.bin";
    }

    static std::string getModelPath(const std::string& type = "encoder") {
        const char* env = getenv("RADAE_MODEL_PATH");
        if (env) return std::string(env);
        return "/usr/share/radae/models/" + type + "_int8.tflite";
    }
};

// Expected accuracy thresholds
struct AccuracyThresholds {
    static constexpr float FEATURE_MSE_THRESHOLD = 0.01f;
    static constexpr float AUDIO_SNR_THRESHOLD = 20.0f;  // dB
    static constexpr float CORRELATION_THRESHOLD = 0.95f;
};

// Performance benchmarks (based on i.MX 8M Plus @ 1.8GHz)
struct PerformanceBenchmarks {
    // Encoder benchmarks (ms per frame)
    static constexpr float ENCODER_NPU_MAX_TIME = 2.0f;   // Target: <2ms
    static constexpr float ENCODER_CPU_MAX_TIME = 10.0f;  // CPU fallback

    // Decoder benchmarks (ms per frame)
    static constexpr float DECODER_NPU_MAX_TIME = 2.0f;   // Target: <2ms
    static constexpr float DECODER_CPU_MAX_TIME = 10.0f;  // CPU fallback

    // Real-time factor (should be < 1.0 for real-time)
    static constexpr float REALTIME_FACTOR_THRESHOLD = 0.1f;  // 10% of real-time
};

// Test configuration
struct TestConfig {
    bool enable_npu = true;
    bool enable_cpu_fallback = true;
    bool verbose = false;
    int num_threads = 4;
    int num_iterations = 100;

    radae_quantization_t quantization = RADAE_QUANT_INT8;

    std::string model_encoder = TestVectors::getModelPath("encoder");
    std::string model_decoder = TestVectors::getModelPath("decoder");
};

// Sample test audio (1 second of 440Hz sine wave at 8kHz)
inline std::vector<int16_t> getTestSineWave() {
    const int sample_rate = 8000;
    const float freq = 440.0f;
    const float duration = 1.0f;
    const int num_samples = static_cast<int>(sample_rate * duration);

    std::vector<int16_t> samples(num_samples);
    for (int i = 0; i < num_samples; i++) {
        float t = static_cast<float>(i) / sample_rate;
        float val = 0.5f * sin(2.0f * M_PI * freq * t);
        samples[i] = static_cast<int16_t>(val * 32767.0f);
    }
    return samples;
}

// Sample white noise
inline std::vector<int16_t> getTestWhiteNoise(size_t length = 8000) {
    std::vector<int16_t> samples(length);
    for (size_t i = 0; i < length; i++) {
        samples[i] = static_cast<int16_t>((rand() % 65536) - 32768);
    }
    return samples;
}

} // namespace test
} // namespace radae

#endif // TEST_DATA_H
