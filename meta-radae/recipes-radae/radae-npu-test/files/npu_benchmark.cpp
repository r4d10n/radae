#include <radae/radae_npu.h>
#include "test_utils.h"
#include "test_data.h"

#include <iostream>
#include <iomanip>
#include <cstring>
#include <cmath>

using namespace radae::test;

void printUsage(const char* prog) {
    std::cout << "Usage: " << prog << " [OPTIONS]\n";
    std::cout << "\nOptions:\n";
    std::cout << "  --quick          Run quick benchmark (10 iterations)\n";
    std::cout << "  --full           Run full benchmark (1000 iterations)\n";
    std::cout << "  --npu-only       Test NPU only (no CPU fallback)\n";
    std::cout << "  --cpu-only       Test CPU only\n";
    std::cout << "  --threads N      Number of threads (default: 4)\n";
    std::cout << "  --help           Show this help\n";
}

void runEncoderBenchmark(const TestConfig& config) {
    std::cout << "\n=== Encoder Benchmark ===\n";

    // Create encoder
    radae_config_t radae_config;
    radae_get_default_config(&radae_config);
    radae_config.exec_mode = config.enable_npu ? RADAE_EXEC_AUTO : RADAE_EXEC_CPU;
    radae_config.num_threads = config.num_threads;
    radae_config.quantization = config.quantization;
    radae_config.model_path = config.model_encoder.c_str();

    radae_error_t error;
    radae_encoder_t encoder = radae_encoder_create(&radae_config, &error);
    if (!encoder) {
        std::cerr << "Failed to create encoder: " << radae_get_error_string(error) << "\n";
        return;
    }

    // Generate test audio
    auto test_audio = getTestSineWave();

    // Warmup
    std::vector<float> features(RADAE_FEATURE_DIM);
    for (int i = 0; i < 10; i++) {
        radae_encoder_process(encoder, test_audio.data(), features.data());
    }

    // Benchmark
    Statistics stats;
    Timer timer;

    std::cout << "Running " << config.num_iterations << " iterations...\n";
    ProgressBar progress(config.num_iterations);

    for (int i = 0; i < config.num_iterations; i++) {
        timer.start();
        error = radae_encoder_process(encoder, test_audio.data(), features.data());
        double elapsed = timer.elapsed_us();

        if (error != RADAE_OK) {
            std::cerr << "Encoding failed: " << radae_get_error_string(error) << "\n";
            break;
        }

        stats.add(elapsed / 1000.0);  // Convert to ms
        progress.update(i + 1);
    }
    progress.finish();

    // Get statistics
    radae_stats_t radae_stats;
    radae_encoder_get_stats(encoder, &radae_stats);

    // Print results
    std::cout << "\nResults:\n";
    std::cout << "  Iterations:     " << config.num_iterations << "\n";
    std::cout << "  Mean time:      " << std::fixed << std::setprecision(3)
              << stats.mean() << " ms\n";
    std::cout << "  Min time:       " << stats.min() << " ms\n";
    std::cout << "  Max time:       " << stats.max() << " ms\n";
    std::cout << "  Std dev:        " << stats.stddev() << " ms\n";
    std::cout << "  NPU usage:      " << std::setprecision(1)
              << (radae_stats.npu_utilization * 100.0f) << "%\n";

    // Real-time factor (frame is 20ms)
    float rtf = stats.mean() / 20.0;
    std::cout << "  RT factor:      " << std::setprecision(3) << rtf;
    if (rtf < 1.0) {
        std::cout << " (PASS - Real-time capable)\n";
    } else {
        std::cout << " (FAIL - Not real-time)\n";
    }

    // Check against benchmark
    bool meets_benchmark = stats.mean() < (config.enable_npu ?
        PerformanceBenchmarks::ENCODER_NPU_MAX_TIME :
        PerformanceBenchmarks::ENCODER_CPU_MAX_TIME);

    std::cout << "  Benchmark:      " << (meets_benchmark ? "PASS" : "FAIL") << "\n";

    radae_encoder_destroy(encoder);
}

void runDecoderBenchmark(const TestConfig& config) {
    std::cout << "\n=== Decoder Benchmark ===\n";

    // Create decoder
    radae_config_t radae_config;
    radae_get_default_config(&radae_config);
    radae_config.exec_mode = config.enable_npu ? RADAE_EXEC_AUTO : RADAE_EXEC_CPU;
    radae_config.num_threads = config.num_threads;
    radae_config.quantization = config.quantization;
    radae_config.model_path = config.model_decoder.c_str();

    radae_error_t error;
    radae_decoder_t decoder = radae_decoder_create(&radae_config, &error);
    if (!decoder) {
        std::cerr << "Failed to create decoder: " << radae_get_error_string(error) << "\n";
        return;
    }

    // Generate test features (random for benchmark)
    std::vector<float> features(RADAE_FEATURE_DIM);
    for (float& f : features) {
        f = (rand() / (float)RAND_MAX) * 2.0f - 1.0f;
    }

    // Warmup
    std::vector<int16_t> audio(RADAE_FRAME_SIZE);
    for (int i = 0; i < 10; i++) {
        radae_decoder_process(decoder, features.data(), audio.data());
    }

    // Benchmark
    Statistics stats;
    Timer timer;

    std::cout << "Running " << config.num_iterations << " iterations...\n";
    ProgressBar progress(config.num_iterations);

    for (int i = 0; i < config.num_iterations; i++) {
        timer.start();
        error = radae_decoder_process(decoder, features.data(), audio.data());
        double elapsed = timer.elapsed_us();

        if (error != RADAE_OK) {
            std::cerr << "Decoding failed: " << radae_get_error_string(error) << "\n";
            break;
        }

        stats.add(elapsed / 1000.0);
        progress.update(i + 1);
    }
    progress.finish();

    // Get statistics
    radae_stats_t radae_stats;
    radae_decoder_get_stats(decoder, &radae_stats);

    // Print results
    std::cout << "\nResults:\n";
    std::cout << "  Iterations:     " << config.num_iterations << "\n";
    std::cout << "  Mean time:      " << std::fixed << std::setprecision(3)
              << stats.mean() << " ms\n";
    std::cout << "  Min time:       " << stats.min() << " ms\n";
    std::cout << "  Max time:       " << stats.max() << " ms\n";
    std::cout << "  Std dev:        " << stats.stddev() << " ms\n";
    std::cout << "  NPU usage:      " << std::setprecision(1)
              << (radae_stats.npu_utilization * 100.0f) << "%\n";

    float rtf = stats.mean() / 20.0;
    std::cout << "  RT factor:      " << std::setprecision(3) << rtf;
    if (rtf < 1.0) {
        std::cout << " (PASS - Real-time capable)\n";
    } else {
        std::cout << " (FAIL - Not real-time)\n";
    }

    bool meets_benchmark = stats.mean() < (config.enable_npu ?
        PerformanceBenchmarks::DECODER_NPU_MAX_TIME :
        PerformanceBenchmarks::DECODER_CPU_MAX_TIME);

    std::cout << "  Benchmark:      " << (meets_benchmark ? "PASS" : "FAIL") << "\n";

    radae_decoder_destroy(decoder);
}

int main(int argc, char** argv) {
    TestConfig config;
    config.num_iterations = 100;  // Default

    // Parse arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "--quick") {
            config.num_iterations = 10;
        } else if (arg == "--full") {
            config.num_iterations = 1000;
        } else if (arg == "--npu-only") {
            config.enable_npu = true;
            config.enable_cpu_fallback = false;
        } else if (arg == "--cpu-only") {
            config.enable_npu = false;
        } else if (arg == "--threads" && i + 1 < argc) {
            config.num_threads = atoi(argv[++i]);
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            printUsage(argv[0]);
            return 1;
        }
    }

    std::cout << "RADAE NPU Benchmark\n";
    std::cout << "===================\n";
    std::cout << "\nConfiguration:\n";
    std::cout << "  NPU enabled:    " << (config.enable_npu ? "Yes" : "No") << "\n";
    std::cout << "  CPU fallback:   " << (config.enable_cpu_fallback ? "Yes" : "No") << "\n";
    std::cout << "  Threads:        " << config.num_threads << "\n";
    std::cout << "  Quantization:   INT8\n";

    // Check NPU availability
    if (config.enable_npu) {
        if (radae_check_npu_available()) {
            std::cout << "  NPU status:     Available\n";
        } else {
            std::cout << "  NPU status:     Not available";
            if (config.enable_cpu_fallback) {
                std::cout << " (will use CPU)\n";
            } else {
                std::cout << "\n";
                return 1;
            }
        }
    }

    runEncoderBenchmark(config);
    runDecoderBenchmark(config);

    std::cout << "\nBenchmark complete!\n";

    return 0;
}
