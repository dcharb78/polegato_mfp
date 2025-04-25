#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <atomic>

namespace mfp {

class PerformanceMonitor {
public:
    // Constructor and destructor
    PerformanceMonitor(bool enabled = true);
    ~PerformanceMonitor();
    
    // Timer operations
    void startTimer(const std::string& operation);
    void stopTimer(const std::string& operation);
    
    // Metric recording
    void recordMetric(const std::string& name, double value);
    void incrementCounter(const std::string& name, int64_t increment = 1);
    
    // Memory tracking
    void recordMemoryUsage(size_t bytes);
    
    // Report generation
    struct OperationMetrics {
        std::string name;
        uint64_t count;
        double total_time;
        double min_time;
        double max_time;
        double avg_time;
    };
    
    struct PerformanceReport {
        uint64_t total_operations;
        double total_execution_time;
        double average_execution_time;
        size_t peak_memory_usage;
        std::vector<OperationMetrics> operations;
        std::unordered_map<std::string, double> custom_metrics;
        std::unordered_map<std::string, int64_t> counters;
    };
    
    PerformanceReport generateReport() const;
    
    // Control
    void setEnabled(bool enabled);
    bool isEnabled() const;
    void reset();
    
private:
    // Internal state
    bool enabled_;
    
    // Timer tracking
    struct TimerInfo {
        std::chrono::high_resolution_clock::time_point start_time;
        bool running;
    };
    
    std::unordered_map<std::string, TimerInfo> active_timers_;
    
    // Metrics storage
    struct OperationData {
        uint64_t count;
        double total_time;
        double min_time;
        double max_time;
    };
    
    std::unordered_map<std::string, OperationData> operation_data_;
    std::unordered_map<std::string, double> custom_metrics_;
    std::unordered_map<std::string, int64_t> counters_;
    
    // Memory tracking
    size_t peak_memory_usage_;
    
    // Synchronization
    mutable std::mutex mutex_;
};

} // namespace mfp
