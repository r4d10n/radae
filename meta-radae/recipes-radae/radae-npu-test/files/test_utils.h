#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <radae/radae_npu.h>
#include <vector>
#include <string>
#include <chrono>

namespace radae {
namespace test {

// Test result structure
struct TestResult {
    std::string name;
    bool passed;
    std::string message;
    double duration_ms;
};

// Test suite class
class TestSuite {
public:
    TestSuite(const std::string& name) : name_(name) {}

    void addResult(const TestResult& result) {
        results_.push_back(result);
    }

    void printSummary() const;

    int getPassCount() const;
    int getFailCount() const;
    bool allPassed() const;

private:
    std::string name_;
    std::vector<TestResult> results_;
};

// Utility functions
std::vector<int16_t> loadAudioFile(const std::string& path);
std::vector<float> loadFeatureFile(const std::string& path);
bool saveAudioFile(const std::string& path, const std::vector<int16_t>& data);
bool saveFeatureFile(const std::string& path, const std::vector<float>& data);

// Generate test signals
std::vector<int16_t> generateSineWave(float freq, float duration, int sample_rate = 8000);
std::vector<int16_t> generateWhiteNoise(size_t length);
std::vector<int16_t> generateSweep(float f0, float f1, float duration, int sample_rate = 8000);

// Signal analysis
float computePESQ(const std::vector<int16_t>& ref, const std::vector<int16_t>& deg);
float computeRMSE(const std::vector<float>& a, const std::vector<float>& b);
float computeCorrelation(const std::vector<float>& a, const std::vector<float>& b);

// Timer helper
class Timer {
public:
    Timer() { start(); }

    void start() {
        start_ = std::chrono::high_resolution_clock::now();
    }

    double elapsed_ms() const {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end - start_).count();
    }

    double elapsed_us() const {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::micro>(end - start_).count();
    }

private:
    std::chrono::high_resolution_clock::time_point start_;
};

// Progress bar
class ProgressBar {
public:
    ProgressBar(size_t total, size_t width = 50)
        : total_(total), width_(width), current_(0) {}

    void update(size_t current);
    void finish();

private:
    size_t total_;
    size_t width_;
    size_t current_;
};

// Statistics tracker
class Statistics {
public:
    void add(double value);
    void reset();

    double mean() const;
    double min() const;
    double max() const;
    double stddev() const;
    size_t count() const { return count_; }

private:
    double sum_ = 0.0;
    double sum_sq_ = 0.0;
    double min_ = INFINITY;
    double max_ = -INFINITY;
    size_t count_ = 0;
};

} // namespace test
} // namespace radae

#endif // TEST_UTILS_H
