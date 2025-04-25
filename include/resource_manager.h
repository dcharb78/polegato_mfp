#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <map>
#include <chrono>

#include "cpu_detector.h"
#include "memory_storage_detector.h"
#include "gpu_detector.h"
#include "cuda_accelerator.h"
#include "metal_accelerator.h"
#include "mfp_base.h"

namespace mfp {
namespace resource {

// Forward declarations
class ResourceManager;
class ExecutionStrategy;
class CPUStrategy;
class CUDAStrategy;
class MetalStrategy;
class HybridStrategy;

// Resource allocation mode
enum class AllocationMode {
    AUTO,           // Automatically select best resources
    CPU_ONLY,       // Use only CPU resources
    GPU_ONLY,       // Use only GPU resources (any available)
    CUDA_ONLY,      // Use only CUDA GPU resources
    METAL_ONLY,     // Use only Metal GPU resources
    HYBRID          // Use both CPU and GPU resources
};

// MFP method to use
enum class MFPMethod {
    AUTO,           // Automatically select best method
    METHOD_1,       // Expanded q Factorization
    METHOD_2,       // Ultrafast with Structural Filter
    METHOD_3        // Parallelized with Dynamic Blocks
};

// Resource allocation result
struct AllocationResult {
    bool success;
    std::string device_name;
    std::string device_type;
    int cores_or_compute_units;
    size_t memory_allocated_bytes;
    std::string error_message;
};

// Performance benchmark result
struct BenchmarkResult {
    double cpu_score;
    double cuda_score;
    double metal_score;
    std::string best_device;
    std::string details;
};

// Execution strategy interface
class ExecutionStrategy {
public:
    virtual ~ExecutionStrategy() = default;
    
    // Initialize strategy
    virtual bool initialize() = 0;
    
    // Check if strategy is available
    virtual bool isAvailable() const = 0;
    
    // Run MFP method
    virtual bool runMFP(MFPMethod method, const std::string& number, std::vector<std::string>& factors) = 0;
    
    // Check if number is prime
    virtual bool isPrime(const std::string& number) = 0;
    
    // Find next prime
    virtual std::string findNextPrime(const std::string& number) = 0;
    
    // Find prime factors
    virtual bool findPrimeFactors(const std::string& number, std::vector<std::string>& factors) = 0;
    
    // Get performance metrics
    virtual std::string getPerformanceMetrics() const = 0;
    
    // Get device information
    virtual std::string getDeviceInfo() const = 0;
    
    // Run benchmark
    virtual double runBenchmark() = 0;
    
    // Get strategy name
    virtual std::string getName() const = 0;
};

// CPU execution strategy
class CPUStrategy : public ExecutionStrategy {
public:
    CPUStrategy(const system::CPUInfo& cpu_info, const system::MemoryInfo& memory_info);
    ~CPUStrategy() override = default;
    
    bool initialize() override;
    bool isAvailable() const override;
    bool runMFP(MFPMethod method, const std::string& number, std::vector<std::string>& factors) override;
    bool isPrime(const std::string& number) override;
    std::string findNextPrime(const std::string& number) override;
    bool findPrimeFactors(const std::string& number, std::vector<std::string>& factors) override;
    std::string getPerformanceMetrics() const override;
    std::string getDeviceInfo() const override;
    double runBenchmark() override;
    std::string getName() const override;
    
private:
    system::CPUInfo cpu_info_;
    system::MemoryInfo memory_info_;
    bool initialized_;
    bool performance_logging_enabled_;
    
    // CPU-specific implementation details
    int optimal_thread_count_;
    
    // Performance metrics
    struct PerformanceMetrics {
        double execution_time_ms;
        size_t memory_used_bytes;
        int threads_used;
        std::string method_name;
    };
    
    std::vector<PerformanceMetrics> performance_metrics_;
    
    // Helper methods
    void logPerformance(const PerformanceMetrics& metrics);
    int calculateOptimalThreadCount() const;
};

// CUDA execution strategy
class CUDAStrategy : public ExecutionStrategy {
public:
    CUDAStrategy(const system::GPUInfo& gpu_info);
    ~CUDAStrategy() override = default;
    
    bool initialize() override;
    bool isAvailable() const override;
    bool runMFP(MFPMethod method, const std::string& number, std::vector<std::string>& factors) override;
    bool isPrime(const std::string& number) override;
    std::string findNextPrime(const std::string& number) override;
    bool findPrimeFactors(const std::string& number, std::vector<std::string>& factors) override;
    std::string getPerformanceMetrics() const override;
    std::string getDeviceInfo() const override;
    double runBenchmark() override;
    std::string getName() const override;
    
private:
    system::GPUInfo gpu_info_;
    std::unique_ptr<cuda::MFPCUDA> cuda_impl_;
    bool initialized_;
};

// Metal execution strategy
class MetalStrategy : public ExecutionStrategy {
public:
    MetalStrategy(const system::GPUInfo& gpu_info);
    ~MetalStrategy() override = default;
    
    bool initialize() override;
    bool isAvailable() const override;
    bool runMFP(MFPMethod method, const std::string& number, std::vector<std::string>& factors) override;
    bool isPrime(const std::string& number) override;
    std::string findNextPrime(const std::string& number) override;
    bool findPrimeFactors(const std::string& number, std::vector<std::string>& factors) override;
    std::string getPerformanceMetrics() const override;
    std::string getDeviceInfo() const override;
    double runBenchmark() override;
    std::string getName() const override;
    
private:
    system::GPUInfo gpu_info_;
    std::unique_ptr<metal::MFPMetal> metal_impl_;
    bool initialized_;
};

// Hybrid execution strategy (uses both CPU and GPU)
class HybridStrategy : public ExecutionStrategy {
public:
    HybridStrategy(std::shared_ptr<CPUStrategy> cpu_strategy, 
                  std::shared_ptr<ExecutionStrategy> gpu_strategy);
    ~HybridStrategy() override = default;
    
    bool initialize() override;
    bool isAvailable() const override;
    bool runMFP(MFPMethod method, const std::string& number, std::vector<std::string>& factors) override;
    bool isPrime(const std::string& number) override;
    std::string findNextPrime(const std::string& number) override;
    bool findPrimeFactors(const std::string& number, std::vector<std::string>& factors) override;
    std::string getPerformanceMetrics() const override;
    std::string getDeviceInfo() const override;
    double runBenchmark() override;
    std::string getName() const override;
    
private:
    std::shared_ptr<CPUStrategy> cpu_strategy_;
    std::shared_ptr<ExecutionStrategy> gpu_strategy_;
    bool initialized_;
    
    // Work distribution parameters
    double cpu_workload_ratio_;
    double gpu_workload_ratio_;
    
    // Helper methods
    void optimizeWorkDistribution();
};

// Resource manager class
class ResourceManager {
public:
    // Constructor and destructor
    ResourceManager();
    ~ResourceManager();
    
    // Initialize resource manager
    bool initialize();
    
    // Set allocation mode
    void setAllocationMode(AllocationMode mode);
    
    // Get allocation mode
    AllocationMode getAllocationMode() const;
    
    // Set MFP method
    void setMFPMethod(MFPMethod method);
    
    // Get MFP method
    MFPMethod getMFPMethod() const;
    
    // Set performance logging
    void setPerformanceLogging(bool enable);
    
    // Get performance logging
    bool getPerformanceLogging() const;
    
    // Allocate resources
    AllocationResult allocateResources(size_t required_memory_bytes = 0);
    
    // Release resources
    void releaseResources();
    
    // Run MFP method
    bool runMFP(const std::string& number, std::vector<std::string>& factors);
    
    // Check if number is prime
    bool isPrime(const std::string& number);
    
    // Find next prime
    std::string findNextPrime(const std::string& number);
    
    // Find prime factors
    bool findPrimeFactors(const std::string& number, std::vector<std::string>& factors);
    
    // Get performance metrics
    std::string getPerformanceMetrics() const;
    
    // Get system information
    std::string getSystemInfo() const;
    
    // Run benchmark
    BenchmarkResult runBenchmark();
    
private:
    // System information
    system::CPUInfo cpu_info_;
    system::MemoryInfo memory_info_;
    std::vector<system::GPUInfo> gpu_info_;
    
    // Configuration
    AllocationMode allocation_mode_;
    MFPMethod mfp_method_;
    bool performance_logging_enabled_;
    
    // Execution strategies
    std::shared_ptr<CPUStrategy> cpu_strategy_;
    std::shared_ptr<CUDAStrategy> cuda_strategy_;
    std::shared_ptr<MetalStrategy> metal_strategy_;
    std::shared_ptr<HybridStrategy> hybrid_strategy_;
    
    // Current strategy
    std::shared_ptr<ExecutionStrategy> current_strategy_;
    
    // Helper methods
    std::shared_ptr<ExecutionStrategy> selectBestStrategy(size_t required_memory_bytes);
    std::shared_ptr<ExecutionStrategy> createStrategyForMode(AllocationMode mode);
    MFPMethod selectBestMethod(const std::string& number);
    bool detectSystemCapabilities();
};

} // namespace resource
} // namespace mfp
