#include <radae/radae_npu.h>
#include "test_utils.h"
#include "test_data.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <cmath>

using namespace radae::test;

void printUsage(const char* prog) {
    std::cout << "Usage: " << prog << " [OPTIONS]\n";
    std::cout << "\nOptions:\n";
    std::cout << "  --test-vectors PATH  Path to test vectors directory\n";
    std::cout << "  --model-encoder PATH Path to encoder model\n";
    std::cout << "  --model-decoder PATH Path to decoder model\n";
    std::cout << "  --verbose            Verbose output\n";
    std::cout << "  --help               Show this help\n";
}

bool testEncoderAccuracy(const TestConfig& config, TestSuite& suite) {
    std::cout << "\n=== Testing Encoder Accuracy ===\n";

    Timer timer;

    // Create encoder
    radae_config_t radae_config;
    radae_get_default_config(&radae_config);
    radae_config.model_path = config.model_encoder.c_str();
    radae_config.quantization = config.quantization;

    radae_error_t error;
    radae_encoder_t encoder = radae_encoder_create(&radae_config, &error);
    if (!encoder) {
        TestResult result;
        result.name = "Encoder Creation";
        result.passed = false;
        result.message = radae_get_error_string(error);
        result.duration_ms = timer.elapsed_ms();
        suite.addResult(result);
        return false;
    }

    TestResult creation_result;
    creation_result.name = "Encoder Creation";
    creation_result.passed = true;
    creation_result.message = "Successfully created encoder";
    creation_result.duration_ms = timer.elapsed_ms();
    suite.addResult(creation_result);

    // Test with sine wave
    timer.start();
    auto sine_wave = generateSineWave(440.0f, 1.0f);

    std::vector<float> features_out(RADAE_FEATURE_DIM);

    // Process first frame
    error = radae_encoder_process(encoder, sine_wave.data(), features_out.data());

    TestResult sine_result;
    sine_result.name = "Sine Wave Processing";
    sine_result.passed = (error == RADAE_OK);
    sine_result.message = error == RADAE_OK ? "Successfully processed sine wave" :
                         radae_get_error_string(error);
    sine_result.duration_ms = timer.elapsed_ms();
    suite.addResult(sine_result);

    // Verify output is not all zeros
    timer.start();
    bool has_nonzero = false;
    for (size_t i = 0; i < RADAE_FEATURE_DIM; i++) {
        if (std::abs(features_out[i]) > 1e-6f) {
            has_nonzero = true;
            break;
        }
    }

    TestResult nonzero_result;
    nonzero_result.name = "Output Non-Zero Check";
    nonzero_result.passed = has_nonzero;
    nonzero_result.message = has_nonzero ? "Output contains valid features" :
                            "Output is all zeros";
    nonzero_result.duration_ms = timer.elapsed_ms();
    suite.addResult(nonzero_result);

    // Verify output range is reasonable
    timer.start();
    float min_val = features_out[0];
    float max_val = features_out[0];
    for (size_t i = 1; i < RADAE_FEATURE_DIM; i++) {
        min_val = std::min(min_val, features_out[i]);
        max_val = std::max(max_val, features_out[i]);
    }

    bool range_ok = (min_val >= -100.0f && max_val <= 100.0f);

    TestResult range_result;
    range_result.name = "Output Range Check";
    range_result.passed = range_ok;
    range_result.message = "Range: [" + std::to_string(min_val) + ", " +
                          std::to_string(max_val) + "]";
    range_result.duration_ms = timer.elapsed_ms();
    suite.addResult(range_result);

    // Test batch processing
    timer.start();
    const int batch_size = 5;
    std::vector<int16_t> audio_batch(batch_size * RADAE_FRAME_SIZE);
    std::vector<float> features_batch(batch_size * RADAE_FEATURE_DIM);

    for (int i = 0; i < batch_size; i++) {
        std::memcpy(audio_batch.data() + i * RADAE_FRAME_SIZE,
                   sine_wave.data(), RADAE_FRAME_SIZE * sizeof(int16_t));
    }

    error = radae_encoder_process_batch(encoder, audio_batch.data(),
                                        features_batch.data(), batch_size);

    TestResult batch_result;
    batch_result.name = "Batch Processing";
    batch_result.passed = (error == RADAE_OK);
    batch_result.message = error == RADAE_OK ? "Successfully processed batch" :
                          radae_get_error_string(error);
    batch_result.duration_ms = timer.elapsed_ms();
    suite.addResult(batch_result);

    radae_encoder_destroy(encoder);

    return true;
}

bool testDecoderAccuracy(const TestConfig& config, TestSuite& suite) {
    std::cout << "\n=== Testing Decoder Accuracy ===\n";

    Timer timer;

    // Create decoder
    radae_config_t radae_config;
    radae_get_default_config(&radae_config);
    radae_config.model_path = config.model_decoder.c_str();
    radae_config.quantization = config.quantization;

    radae_error_t error;
    radae_decoder_t decoder = radae_decoder_create(&radae_config, &error);
    if (!decoder) {
        TestResult result;
        result.name = "Decoder Creation";
        result.passed = false;
        result.message = radae_get_error_string(error);
        result.duration_ms = timer.elapsed_ms();
        suite.addResult(result);
        return false;
    }

    TestResult creation_result;
    creation_result.name = "Decoder Creation";
    creation_result.passed = true;
    creation_result.message = "Successfully created decoder";
    creation_result.duration_ms = timer.elapsed_ms();
    suite.addResult(creation_result);

    // Test with random features
    timer.start();
    std::vector<float> features_in(RADAE_FEATURE_DIM);
    for (size_t i = 0; i < RADAE_FEATURE_DIM; i++) {
        features_in[i] = (rand() / (float)RAND_MAX) * 2.0f - 1.0f;
    }

    std::vector<int16_t> audio_out(RADAE_FRAME_SIZE);
    error = radae_decoder_process(decoder, features_in.data(), audio_out.data());

    TestResult decode_result;
    decode_result.name = "Feature Decoding";
    decode_result.passed = (error == RADAE_OK);
    decode_result.message = error == RADAE_OK ? "Successfully decoded features" :
                           radae_get_error_string(error);
    decode_result.duration_ms = timer.elapsed_ms();
    suite.addResult(decode_result);

    // Verify output is not all zeros
    timer.start();
    bool has_nonzero = false;
    for (size_t i = 0; i < RADAE_FRAME_SIZE; i++) {
        if (audio_out[i] != 0) {
            has_nonzero = true;
            break;
        }
    }

    TestResult nonzero_result;
    nonzero_result.name = "Output Non-Zero Check";
    nonzero_result.passed = has_nonzero;
    nonzero_result.message = has_nonzero ? "Output contains valid audio" :
                            "Output is all zeros";
    nonzero_result.duration_ms = timer.elapsed_ms();
    suite.addResult(nonzero_result);

    // Test batch processing
    timer.start();
    const int batch_size = 5;
    std::vector<float> features_batch(batch_size * RADAE_FEATURE_DIM);
    std::vector<int16_t> audio_batch(batch_size * RADAE_FRAME_SIZE);

    for (int i = 0; i < batch_size; i++) {
        std::memcpy(features_batch.data() + i * RADAE_FEATURE_DIM,
                   features_in.data(), RADAE_FEATURE_DIM * sizeof(float));
    }

    error = radae_decoder_process_batch(decoder, features_batch.data(),
                                        audio_batch.data(), batch_size);

    TestResult batch_result;
    batch_result.name = "Batch Processing";
    batch_result.passed = (error == RADAE_OK);
    batch_result.message = error == RADAE_OK ? "Successfully processed batch" :
                          radae_get_error_string(error);
    batch_result.duration_ms = timer.elapsed_ms();
    suite.addResult(batch_result);

    radae_decoder_destroy(decoder);

    return true;
}

bool testRoundTrip(const TestConfig& config, TestSuite& suite) {
    std::cout << "\n=== Testing Round-Trip Accuracy ===\n";

    Timer timer;

    // Create encoder and decoder
    radae_config_t enc_config, dec_config;
    radae_get_default_config(&enc_config);
    radae_get_default_config(&dec_config);

    enc_config.model_path = config.model_encoder.c_str();
    dec_config.model_path = config.model_decoder.c_str();

    radae_error_t error;
    radae_encoder_t encoder = radae_encoder_create(&enc_config, &error);
    radae_decoder_t decoder = radae_decoder_create(&dec_config, &error);

    if (!encoder || !decoder) {
        TestResult result;
        result.name = "Round-Trip Setup";
        result.passed = false;
        result.message = "Failed to create encoder/decoder";
        result.duration_ms = timer.elapsed_ms();
        suite.addResult(result);

        if (encoder) radae_encoder_destroy(encoder);
        if (decoder) radae_decoder_destroy(decoder);
        return false;
    }

    // Generate test signal
    auto original_audio = generateSineWave(440.0f, 0.02f);  // Single frame

    // Encode
    std::vector<float> features(RADAE_FEATURE_DIM);
    error = radae_encoder_process(encoder, original_audio.data(), features.data());
    if (error != RADAE_OK) {
        TestResult result;
        result.name = "Round-Trip Encode";
        result.passed = false;
        result.message = radae_get_error_string(error);
        result.duration_ms = timer.elapsed_ms();
        suite.addResult(result);

        radae_encoder_destroy(encoder);
        radae_decoder_destroy(decoder);
        return false;
    }

    // Decode
    std::vector<int16_t> reconstructed_audio(RADAE_FRAME_SIZE);
    error = radae_decoder_process(decoder, features.data(), reconstructed_audio.data());
    if (error != RADAE_OK) {
        TestResult result;
        result.name = "Round-Trip Decode";
        result.passed = false;
        result.message = radae_get_error_string(error);
        result.duration_ms = timer.elapsed_ms();
        suite.addResult(result);

        radae_encoder_destroy(encoder);
        radae_decoder_destroy(decoder);
        return false;
    }

    // Compute SNR
    float snr = radae_compute_snr(original_audio.data(), reconstructed_audio.data(),
                                  RADAE_FRAME_SIZE);

    TestResult snr_result;
    snr_result.name = "Round-Trip SNR";
    snr_result.passed = (snr >= AccuracyThresholds::AUDIO_SNR_THRESHOLD);
    snr_result.message = "SNR: " + std::to_string(snr) + " dB (threshold: " +
                        std::to_string(AccuracyThresholds::AUDIO_SNR_THRESHOLD) + " dB)";
    snr_result.duration_ms = timer.elapsed_ms();
    suite.addResult(snr_result);

    if (config.verbose) {
        std::cout << "  SNR: " << std::fixed << std::setprecision(2) << snr << " dB\n";
    }

    radae_encoder_destroy(encoder);
    radae_decoder_destroy(decoder);

    return true;
}

int main(int argc, char** argv) {
    TestConfig config;

    // Parse arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "--test-vectors" && i + 1 < argc) {
            // Test vectors path (not used yet but prepared for future)
            i++;
        } else if (arg == "--model-encoder" && i + 1 < argc) {
            config.model_encoder = argv[++i];
        } else if (arg == "--model-decoder" && i + 1 < argc) {
            config.model_decoder = argv[++i];
        } else if (arg == "--verbose") {
            config.verbose = true;
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            printUsage(argv[0]);
            return 1;
        }
    }

    std::cout << "RADAE NPU Accuracy Test\n";
    std::cout << "========================\n";

    // Check NPU availability
    if (radae_check_npu_available()) {
        std::cout << "NPU Status: Available\n";
    } else {
        std::cout << "NPU Status: Not available (using CPU)\n";
    }

    // Run test suite
    TestSuite suite("RADAE NPU Accuracy");

    testEncoderAccuracy(config, suite);
    testDecoderAccuracy(config, suite);
    testRoundTrip(config, suite);

    // Print summary
    std::cout << "\n";
    suite.printSummary();

    return suite.allPassed() ? 0 : 1;
}
