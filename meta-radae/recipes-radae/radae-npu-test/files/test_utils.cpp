#include "test_utils.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cmath>
#include <algorithm>

namespace radae {
namespace test {

// TestSuite implementation
void TestSuite::printSummary() const {
    std::cout << "Test Suite: " << name_ << "\n";
    std::cout << std::string(60, '=') << "\n";

    int pass_count = 0;
    int fail_count = 0;

    for (const auto& result : results_) {
        std::cout << "[" << (result.passed ? "PASS" : "FAIL") << "] "
                  << std::setw(30) << std::left << result.name
                  << " (" << std::fixed << std::setprecision(2)
                  << result.duration_ms << " ms)\n";
        if (!result.message.empty()) {
            std::cout << "       " << result.message << "\n";
        }

        if (result.passed) {
            pass_count++;
        } else {
            fail_count++;
        }
    }

    std::cout << std::string(60, '=') << "\n";
    std::cout << "Total: " << results_.size()
              << " | Pass: " << pass_count
              << " | Fail: " << fail_count << "\n";

    if (allPassed()) {
        std::cout << "Result: ALL TESTS PASSED\n";
    } else {
        std::cout << "Result: SOME TESTS FAILED\n";
    }
}

int TestSuite::getPassCount() const {
    int count = 0;
    for (const auto& result : results_) {
        if (result.passed) count++;
    }
    return count;
}

int TestSuite::getFailCount() const {
    int count = 0;
    for (const auto& result : results_) {
        if (!result.passed) count++;
    }
    return count;
}

bool TestSuite::allPassed() const {
    for (const auto& result : results_) {
        if (!result.passed) return false;
    }
    return true;
}

// File I/O functions
std::vector<int16_t> loadAudioFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return {};
    }

    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<int16_t> data(size / sizeof(int16_t));
    file.read(reinterpret_cast<char*>(data.data()), size);

    return data;
}

std::vector<float> loadFeatureFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return {};
    }

    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<float> data(size / sizeof(float));
    file.read(reinterpret_cast<char*>(data.data()), size);

    return data;
}

bool saveAudioFile(const std::string& path, const std::vector<int16_t>& data) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }

    file.write(reinterpret_cast<const char*>(data.data()),
               data.size() * sizeof(int16_t));
    return file.good();
}

bool saveFeatureFile(const std::string& path, const std::vector<float>& data) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }

    file.write(reinterpret_cast<const char*>(data.data()),
               data.size() * sizeof(float));
    return file.good();
}

// Signal generation
std::vector<int16_t> generateSineWave(float freq, float duration, int sample_rate) {
    int num_samples = static_cast<int>(sample_rate * duration);
    std::vector<int16_t> samples(num_samples);

    for (int i = 0; i < num_samples; i++) {
        float t = static_cast<float>(i) / sample_rate;
        float val = 0.5f * sin(2.0f * M_PI * freq * t);
        samples[i] = static_cast<int16_t>(val * 32767.0f);
    }

    return samples;
}

std::vector<int16_t> generateWhiteNoise(size_t length) {
    std::vector<int16_t> samples(length);
    for (size_t i = 0; i < length; i++) {
        samples[i] = static_cast<int16_t>((rand() % 65536) - 32768) / 4;
    }
    return samples;
}

std::vector<int16_t> generateSweep(float f0, float f1, float duration, int sample_rate) {
    int num_samples = static_cast<int>(sample_rate * duration);
    std::vector<int16_t> samples(num_samples);

    float k = (f1 - f0) / duration;

    for (int i = 0; i < num_samples; i++) {
        float t = static_cast<float>(i) / sample_rate;
        float freq = f0 + k * t;
        float val = 0.5f * sin(2.0f * M_PI * freq * t);
        samples[i] = static_cast<int16_t>(val * 32767.0f);
    }

    return samples;
}

// Signal analysis
float computePESQ(const std::vector<int16_t>& ref, const std::vector<int16_t>& deg) {
    // Simplified PESQ approximation (not actual PESQ algorithm)
    // Real PESQ requires ITU-T P.862 implementation
    if (ref.size() != deg.size()) return -1.0f;

    // Compute correlation as a simple quality metric
    double sum_ref = 0.0, sum_deg = 0.0, sum_prod = 0.0;
    double sum_ref_sq = 0.0, sum_deg_sq = 0.0;

    for (size_t i = 0; i < ref.size(); i++) {
        double r = ref[i] / 32768.0;
        double d = deg[i] / 32768.0;

        sum_ref += r;
        sum_deg += d;
        sum_prod += r * d;
        sum_ref_sq += r * r;
        sum_deg_sq += d * d;
    }

    size_t n = ref.size();
    double correlation = (n * sum_prod - sum_ref * sum_deg) /
                        sqrt((n * sum_ref_sq - sum_ref * sum_ref) *
                             (n * sum_deg_sq - sum_deg * sum_deg));

    // Map correlation to pseudo-PESQ scale [1.0, 4.5]
    return 1.0f + 3.5f * std::max(0.0, correlation);
}

float computeRMSE(const std::vector<float>& a, const std::vector<float>& b) {
    if (a.size() != b.size()) return -1.0f;

    double sum = 0.0;
    for (size_t i = 0; i < a.size(); i++) {
        double diff = a[i] - b[i];
        sum += diff * diff;
    }

    return sqrt(sum / a.size());
}

float computeCorrelation(const std::vector<float>& a, const std::vector<float>& b) {
    if (a.size() != b.size()) return -1.0f;

    double sum_a = 0.0, sum_b = 0.0, sum_prod = 0.0;
    double sum_a_sq = 0.0, sum_b_sq = 0.0;

    for (size_t i = 0; i < a.size(); i++) {
        sum_a += a[i];
        sum_b += b[i];
        sum_prod += a[i] * b[i];
        sum_a_sq += a[i] * a[i];
        sum_b_sq += b[i] * b[i];
    }

    size_t n = a.size();
    double num = n * sum_prod - sum_a * sum_b;
    double den = sqrt((n * sum_a_sq - sum_a * sum_a) *
                     (n * sum_b_sq - sum_b * sum_b));

    if (den < 1e-10) return 0.0f;

    return num / den;
}

// ProgressBar implementation
void ProgressBar::update(size_t current) {
    current_ = current;

    float progress = static_cast<float>(current_) / total_;
    size_t bar_length = static_cast<size_t>(progress * width_);

    std::cout << "\r[";
    for (size_t i = 0; i < width_; i++) {
        if (i < bar_length) {
            std::cout << "=";
        } else if (i == bar_length) {
            std::cout << ">";
        } else {
            std::cout << " ";
        }
    }
    std::cout << "] " << std::setw(3) << static_cast<int>(progress * 100) << "%";
    std::cout.flush();
}

void ProgressBar::finish() {
    update(total_);
    std::cout << "\n";
}

// Statistics implementation
void Statistics::add(double value) {
    sum_ += value;
    sum_sq_ += value * value;
    min_ = std::min(min_, value);
    max_ = std::max(max_, value);
    count_++;
}

void Statistics::reset() {
    sum_ = 0.0;
    sum_sq_ = 0.0;
    min_ = INFINITY;
    max_ = -INFINITY;
    count_ = 0;
}

double Statistics::mean() const {
    if (count_ == 0) return 0.0;
    return sum_ / count_;
}

double Statistics::min() const {
    return min_;
}

double Statistics::max() const {
    return max_;
}

double Statistics::stddev() const {
    if (count_ < 2) return 0.0;
    double m = mean();
    double variance = (sum_sq_ / count_) - (m * m);
    return sqrt(std::max(0.0, variance));
}

} // namespace test
} // namespace radae
