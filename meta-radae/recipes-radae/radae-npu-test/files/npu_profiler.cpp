#include <radae/radae_npu.h>
#include "test_utils.h"
#include "test_data.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <thread>
#include <atomic>
#include <signal.h>

using namespace radae::test;

std::atomic<bool> running(true);

void signalHandler(int) {
    running = false;
}

void printUsage(const char* prog) {
    std::cout << "Usage: " << prog << " [OPTIONS]\n";
    std::cout << "\nOptions:\n";
    std::cout << "  --duration SECS      Run duration in seconds (default: 60)\n";
    std::cout << "  --stress-mode        Run in stress test mode\n";
    std::cout << "  --log FILE           Log results to file\n";
    std::cout << "  --interval MS        Reporting interval in ms (default: 1000)\n";
    std::cout << "  --help               Show this help\n";
}

struct ProfilingResult {
    double timestamp;
    double encode_time_ms;
    double decode_time_ms;
    bool npu_active;
    size_t total_frames;
};

void runProfilingSession(const TestConfig& config, int duration_secs,
                        int interval_ms, const std::string& log_file,
                        bool stress_mode) {
    std::cout << "\nStarting profiling session...\n";
    std::cout << "Duration: " << duration_secs << " seconds\n";
    std::cout << "Interval: " << interval_ms << " ms\n";
    if (stress_mode) {
        std::cout << "Mode: STRESS TEST\n";
    }

    // Create encoder and decoder
    radae_config_t enc_config, dec_config;
    radae_get_default_config(&enc_config);
    radae_get_default_config(&dec_config);

    enc_config.model_path = config.model_encoder.c_str();
    dec_config.model_path = config.model_decoder.c_str();
    enc_config.enable_profiling = 1;
    dec_config.enable_profiling = 1;

    radae_error_t error;
    radae_encoder_t encoder = radae_encoder_create(&enc_config, &error);
    if (!encoder) {
        std::cerr << "Failed to create encoder: " << radae_get_error_string(error) << "\n";
        return;
    }

    radae_decoder_t decoder = radae_decoder_create(&dec_config, &error);
    if (!decoder) {
        std::cerr << "Failed to create decoder: " << radae_get_error_string(error) << "\n";
        radae_encoder_destroy(encoder);
        return;
    }

    // Open log file if specified
    std::ofstream log;
    if (!log_file.empty()) {
        log.open(log_file);
        log << "timestamp,encode_ms,decode_ms,npu_active,total_frames\n";
    }

    // Generate test audio
    auto test_audio = generateSineWave(440.0f, 1.0f);
    std::vector<float> features(RADAE_FEATURE_DIM);
    std::vector<int16_t> audio_out(RADAE_FRAME_SIZE);

    // Statistics
    Statistics encode_stats, decode_stats;
    size_t total_frames = 0;
    size_t errors = 0;

    auto start_time = std::chrono::steady_clock::now();
    auto last_report = start_time;

    std::cout << "\nProfiling...\n";
    std::cout << std::setw(10) << "Time(s)"
              << std::setw(15) << "Frames"
              << std::setw(15) << "Enc(ms)"
              << std::setw(15) << "Dec(ms)"
              << std::setw(12) << "Errors"
              << std::setw(12) << "NPU\n";
    std::cout << std::string(79, '-') << "\n";

    while (running) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time);

        if (elapsed.count() >= duration_secs) {
            break;
        }

        // Encode
        Timer enc_timer;
        error = radae_encoder_process(encoder, test_audio.data(), features.data());
        double enc_time = enc_timer.elapsed_ms();

        if (error != RADAE_OK) {
            errors++;
            continue;
        }

        // Decode
        Timer dec_timer;
        error = radae_decoder_process(decoder, features.data(), audio_out.data());
        double dec_time = dec_timer.elapsed_ms();

        if (error != RADAE_OK) {
            errors++;
            continue;
        }

        // Update statistics
        encode_stats.add(enc_time);
        decode_stats.add(dec_time);
        total_frames++;

        // Report at intervals
        auto since_report = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - last_report);

        if (since_report.count() >= interval_ms) {
            radae_stats_t enc_stats, dec_stats;
            radae_encoder_get_stats(encoder, &enc_stats);
            radae_decoder_get_stats(decoder, &dec_stats);

            bool npu_active = (enc_stats.npu_utilization > 0.5f);

            std::cout << std::fixed << std::setprecision(1)
                      << std::setw(10) << elapsed.count()
                      << std::setw(15) << total_frames
                      << std::setw(15) << encode_stats.mean()
                      << std::setw(15) << decode_stats.mean()
                      << std::setw(12) << errors
                      << std::setw(12) << (npu_active ? "Yes" : "No")
                      << "\n";

            // Log to file
            if (log.is_open()) {
                log << elapsed.count() << ","
                    << encode_stats.mean() << ","
                    << decode_stats.mean() << ","
                    << (npu_active ? 1 : 0) << ","
                    << total_frames << "\n";
            }

            last_report = now;
        }

        // In stress mode, run continuously without sleep
        if (!stress_mode) {
            // Simulate real-time processing (20ms frame)
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }

    std::cout << std::string(79, '-') << "\n";

    // Final statistics
    radae_stats_t enc_stats, dec_stats;
    radae_encoder_get_stats(encoder, &enc_stats);
    radae_decoder_get_stats(decoder, &dec_stats);

    std::cout << "\nFinal Statistics:\n";
    std::cout << "  Total frames:       " << total_frames << "\n";
    std::cout << "  Total errors:       " << errors << "\n";
    std::cout << "  Error rate:         " << std::setprecision(2)
              << (100.0 * errors / (total_frames + errors)) << "%\n";
    std::cout << "\nEncoder:\n";
    std::cout << "  Mean time:          " << std::setprecision(3)
              << encode_stats.mean() << " ms\n";
    std::cout << "  Min time:           " << encode_stats.min() << " ms\n";
    std::cout << "  Max time:           " << encode_stats.max() << " ms\n";
    std::cout << "  Std dev:            " << encode_stats.stddev() << " ms\n";
    std::cout << "  NPU utilization:    " << std::setprecision(1)
              << (enc_stats.npu_utilization * 100.0f) << "%\n";

    std::cout << "\nDecoder:\n";
    std::cout << "  Mean time:          " << std::setprecision(3)
              << decode_stats.mean() << " ms\n";
    std::cout << "  Min time:           " << decode_stats.min() << " ms\n";
    std::cout << "  Max time:           " << decode_stats.max() << " ms\n";
    std::cout << "  Std dev:            " << decode_stats.stddev() << " ms\n";
    std::cout << "  NPU utilization:    " << std::setprecision(1)
              << (dec_stats.npu_utilization * 100.0f) << "%\n";

    // Throughput
    auto total_time = std::chrono::steady_clock::now() - start_time;
    double total_secs = std::chrono::duration<double>(total_time).count();
    double fps = total_frames / total_secs;

    std::cout << "\nThroughput:\n";
    std::cout << "  Frames/second:      " << std::setprecision(1) << fps << "\n";
    std::cout << "  Real-time factor:   " << std::setprecision(3)
              << ((encode_stats.mean() + decode_stats.mean()) / 20.0) << "\n";

    // Cleanup
    radae_encoder_destroy(encoder);
    radae_decoder_destroy(decoder);

    if (log.is_open()) {
        log.close();
        std::cout << "\nResults logged to: " << log_file << "\n";
    }
}

int main(int argc, char** argv) {
    TestConfig config;
    int duration = 60;
    int interval = 1000;
    std::string log_file;
    bool stress_mode = false;

    // Parse arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "--duration" && i + 1 < argc) {
            duration = atoi(argv[++i]);
        } else if (arg == "--stress-mode") {
            stress_mode = true;
        } else if (arg == "--log" && i + 1 < argc) {
            log_file = argv[++i];
        } else if (arg == "--interval" && i + 1 < argc) {
            interval = atoi(argv[++i]);
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            printUsage(argv[0]);
            return 1;
        }
    }

    // Setup signal handler
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    std::cout << "RADAE NPU Profiler\n";
    std::cout << "==================\n";

    if (radae_check_npu_available()) {
        std::cout << "NPU Status: Available\n";
    } else {
        std::cout << "NPU Status: Not available (using CPU)\n";
    }

    runProfilingSession(config, duration, interval, log_file, stress_mode);

    std::cout << "\nProfiler complete!\n";

    return 0;
}
