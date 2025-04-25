#include "resource_manager.h"
#include <sstream>
#include <algorithm>
#include <thread>
#include <iomanip>
#include <cmath>

namespace mfp {
namespace resource {

//=============================================================================
// CPU Strategy Implementation
//=============================================================================

CPUStrategy::CPUStrategy(const system::CPUInfo& cpu_info, const system::MemoryInfo& memory_info)
    : cpu_info_(cpu_info),
      memory_info_(memory_info),
      initialized_(false),
      performance_logging_enabled_(false),
      optimal_thread_count_(0) {
}

bool CPUStrategy::initialize() {
    if (initialized_) {
        return true;
    }
    
    // Calculate optimal thread count based on CPU info
    optimal_thread_count_ = calculateOptimalThreadCount();
    
    initialized_ = true;
    return true;
}

bool CPUStrategy::isAvailable() const {
    return initialized_;
}

bool CPUStrategy::runMFP(MFPMethod method, const std::string& number, std::vector<std::string>& factors) {
    if (!initialized_) {
        return false;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    bool result = false;
    
    // Implement CPU-based MFP methods
    switch (method) {
        case MFPMethod::METHOD_1:
            // TODO: Implement Method 1 on CPU
            result = true;
            break;
            
        case MFPMethod::METHOD_2:
            // TODO: Implement Method 2 on CPU
            result = true;
            break;
            
        case MFPMethod::METHOD_3:
            // TODO: Implement Method 3 on CPU
            result = true;
            break;
            
        case MFPMethod::AUTO:
            // For AUTO, choose the best method based on number size
            if (number.size() < 100) {
                // For small numbers, use Method 1
                // TODO: Implement Method 1 on CPU
                result = true;
            } else if (number.size() < 1000) {
                // For medium numbers, use Method 2
                // TODO: Implement Method 2 on CPU
                result = true;
            } else {
                // For large numbers, use Method 3
                // TODO: Implement Method 3 on CPU
                result = true;
            }
            break;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    
    if (performance_logging_enabled_) {
        PerformanceMetrics metrics;
        metrics.execution_time_ms = duration;
        metrics.memory_used_bytes = number.size() * 10; // Estimate memory usage
        metrics.threads_used = optimal_thread_count_;
        
        switch (method) {
            case MFPMethod::METHOD_1:
                metrics.method_name = "CPU_Method1_ExpandedQFactorization";
                break;
                
            case MFPMethod::METHOD_2:
                metrics.method_name = "CPU_Method2_UltrafastWithStructuralFilter";
                break;
                
            case MFPMethod::METHOD_3:
                metrics.method_name = "CPU_Method3_ParallelizedWithDynamicBlocks";
                break;
                
            case MFPMethod::AUTO:
                metrics.method_name = "CPU_MethodAuto";
                break;
        }
        
        logPerformance(metrics);
    }
    
    return result;
}

bool CPUStrategy::isPrime(const std::string& number) {
    if (!initialized_) {
        return false;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // TODO: Implement isPrime on CPU
    bool result = true;
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    
    if (performance_logging_enabled_) {
        PerformanceMetrics metrics;
        metrics.execution_time_ms = duration;
        metrics.memory_used_bytes = number.size() * 2; // Estimate memory usage
        metrics.threads_used = optimal_thread_count_;
        metrics.method_name = "CPU_IsPrime";
        
        logPerformance(metrics);
    }
    
    return result;
}

std::string CPUStrategy::findNextPrime(const std::string& number) {
    if (!initialized_) {
        return "";
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // TODO: Implement findNextPrime on CPU
    std::string result = number;
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    
    if (performance_logging_enabled_) {
        PerformanceMetrics metrics;
        metrics.execution_time_ms = duration;
        metrics.memory_used_bytes = number.size() * 3; // Estimate memory usage
        metrics.threads_used = optimal_thread_count_;
        metrics.method_name = "CPU_FindNextPrime";
        
        logPerformance(metrics);
    }
    
    return result;
}

bool CPUStrategy::findPrimeFactors(const std::string& number, std::vector<std::string>& factors) {
    if (!initialized_) {
        return false;
    }
    
    // Use Method 3 by default for prime factorization
    return runMFP(MFPMethod::METHOD_3, number, factors);
}

std::string CPUStrategy::getPerformanceMetrics() const {
    if (performance_metrics_.empty()) {
        return "No performance metrics available for CPU strategy";
    }
    
    std::stringstream ss;
    ss << "CPU Strategy Performance Metrics:\n";
    
    for (const auto& metrics : performance_metrics_) {
        ss << "Method: " << metrics.method_name << "\n";
        ss << "  Execution Time: " << metrics.execution_time_ms << " ms\n";
        ss << "  Memory Used: " << (metrics.memory_used_bytes / (1024.0 * 1024.0)) << " MB\n";
        ss << "  Threads Used: " << metrics.threads_used << "\n";
        ss << "\n";
    }
    
    return ss.str();
}

std::string CPUStrategy::getDeviceInfo() const {
    std::stringstream ss;
    ss << "CPU: " << cpu_info_.model_name << "\n";
    ss << "  Architecture: " << cpu_info_.architecture << "\n";
    ss << "  Physical Cores: " << cpu_info_.physical_cores << "\n";
    ss << "  Logical Cores: " << cpu_info_.logical_cores << "\n";
    ss << "  Base Frequency: " << cpu_info_.base_frequency_mhz << " MHz\n";
    
    if (!cpu_info_.features.empty()) {
        ss << "  Features: ";
        for (size_t i = 0; i < cpu_info_.features.size(); ++i) {
            if (i > 0) {
                ss << ", ";
            }
            ss << cpu_info_.features[i];
        }
        ss << "\n";
    }
    
    ss << "  L1 Cache: " << (cpu_info_.cache_sizes[0] / 1024) << " KB\n";
    ss << "  L2 Cache: " << (cpu_info_.cache_sizes[1] / 1024) << " KB\n";
    ss << "  L3 Cache: " << (cpu_info_.cache_sizes[2] / (1024 * 1024)) << " MB\n";
    
    return ss.str();
}

double CPUStrategy::runBenchmark() {
    if (!initialized_) {
        return 0.0;
    }
    
    // Run a simple benchmark to measure CPU performance
    const int iterations = 10;
    double total_score = 0.0;
    
    for (int i = 0; i < iterations; ++i) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Perform a compute-intensive task
        // TODO: Implement a more realistic benchmark
        volatile double result = 0.0;
        for (int j = 0; j < 10000000; ++j) {
            result += std::sqrt(j);
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        
        // Calculate score (higher is better)
        double score = 10000.0 / duration;
        total_score += score;
    }
    
    return total_score / iterations;
}

std::string CPUStrategy::getName() const {
    return "CPU";
}

void CPUStrategy::logPerformance(const PerformanceMetrics& metrics) {
    performance_metrics_.push_back(metrics);
}

int CPUStrategy::calculateOptimalThreadCount() const {
    // Start with the number of logical cores
    int thread_count = cpu_info_.logical_cores;
    
    // If hyper-threading is enabled, we might want to use fewer threads
    // to avoid contention
    if (cpu_info_.logical_cores > cpu_info_.physical_cores) {
        // Use 75% of logical cores if hyper-threading is enabled
        thread_count = std::max(cpu_info_.physical_cores, 
                               static_cast<int>(cpu_info_.logical_cores * 0.75));
    }
    
    // Ensure we have at least one thread
    return std::max(1, thread_count);
}

//=============================================================================
// CUDA Strategy Implementation
//=============================================================================

CUDAStrategy::CUDAStrategy(const system::GPUInfo& gpu_info)
    : gpu_info_(gpu_info),
      initialized_(false) {
    cuda_impl_ = std::make_unique<cuda::MFPCUDA>();
}

bool CUDAStrategy::initialize() {
    if (initialized_) {
        return true;
    }
    
    // Initialize CUDA implementation
    if (!cuda_impl_->initialize(gpu_info_.device_id)) {
        return false;
    }
    
    initialized_ = true;
    return true;
}

bool CUDAStrategy::isAvailable() const {
    return initialized_ && cuda_impl_->isAvailable();
}

bool CUDAStrategy::runMFP(MFPMethod method, const std::string& number, std::vector<std::string>& factors) {
    if (!initialized_ || !cuda_impl_->isAvailable()) {
        return false;
    }
    
    switch (method) {
        case MFPMethod::METHOD_1:
            return cuda_impl_->runMethod1(number, factors);
            
        case MFPMethod::METHOD_2:
            return cuda_impl_->runMethod2(number, factors);
            
        case MFPMethod::METHOD_3:
            return cuda_impl_->runMethod3(number, factors);
            
        case MFPMethod::AUTO:
            // For AUTO, choose the best method based on number size
            if (number.size() < 100) {
                // For small numbers, use Method 1
                return cuda_impl_->runMethod1(number, factors);
            } else if (number.size() < 1000) {
                // For medium numbers, use Method 2
                return cuda_impl_->runMethod2(number, factors);
            } else {
                // For large numbers, use Method 3
                return cuda_impl_->runMethod3(number, factors);
            }
    }
    
    return false;
}

bool CUDAStrategy::isPrime(const std::string& number) {
    if (!initialized_ || !cuda_impl_->isAvailable()) {
        return false;
    }
    
    return cuda_impl_->isPrime(number);
}

std::string CUDAStrategy::findNextPrime(const std::string& number) {
    if (!initialized_ || !cuda_impl_->isAvailable()) {
        return "";
    }
    
    return cuda_impl_->findNextPrime(number);
}

bool CUDAStrategy::findPrimeFactors(const std::string& number, std::vector<std::string>& factors) {
    if (!initialized_ || !cuda_impl_->isAvailable()) {
        return false;
    }
    
    return cuda_impl_->findPrimeFactors(number, factors);
}

std::string CUDAStrategy::getPerformanceMetrics() const {
    if (!initialized_ || !cuda_impl_->isAvailable()) {
        return "No performance metrics available for CUDA strategy";
    }
    
    return cuda_impl_->getPerformanceMetrics();
}

std::string CUDAStrategy::getDeviceInfo() const {
    std::stringstream ss;
    ss << "CUDA GPU: " << gpu_info_.name << "\n";
    ss << "  Vendor: " << system::GPUVendorToString(gpu_info_.vendor) << "\n";
    ss << "  Architecture: " << system::GPUArchitectureToString(gpu_info_.architecture) << "\n";
    
    if (!gpu_info_.api_support.empty()) {
        ss << "  API Support: ";
        for (size_t i = 0; i < gpu_info_.api_support.size(); ++i) {
            if (i > 0) {
                ss << ", ";
            }
            ss << system::GPUAPISupportToString(gpu_info_.api_support[i]);
        }
        ss << "\n";
    }
    
    ss << "  Memory: " << (gpu_info_.memory_info.total_memory_bytes / (1024 * 1024 * 1024)) << " GB\n";
    ss << "  Memory Bandwidth: " << gpu_info_.memory_info.memory_bandwidth_gbps << " GB/s\n";
    ss << "  CUDA Cores: " << gpu_info_.compute_info.cuda_cores << "\n";
    ss << "  Core Clock: " << gpu_info_.compute_info.core_clock_mhz << " MHz\n";
    ss << "  Compute Capability: " << gpu_info_.compute_info.cuda_compute_capability << "\n";
    ss << "  Theoretical Performance: " << gpu_info_.compute_info.theoretical_tflops_fp32 << " TFLOPS (FP32)\n";
    
    return ss.str();
}

double CUDAStrategy::runBenchmark() {
    if (!initialized_ || !cuda_impl_->isAvailable()) {
        return 0.0;
    }
    
    // Run a simple benchmark to measure CUDA performance
    const int iterations = 5;
    double total_score = 0.0;
    
    for (int i = 0; i < iterations; ++i) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Perform a compute-intensive task on GPU
        // TODO: Implement a more realistic benchmark
        std::vector<std::string> factors;
        std::string test_number = "1234567890123456789012345678901234567890";
        cuda_impl_->runMethod3(test_number, factors);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        
        // Calculate score (higher is better)
        double score = 10000.0 / duration;
        total_score += score;
    }
    
    return total_score / iterations;
}

std::string CUDAStrategy::getName() const {
    return "CUDA";
}

//=============================================================================
// Metal Strategy Implementation
//=============================================================================

MetalStrategy::MetalStrategy(const system::GPUInfo& gpu_info)
    : gpu_info_(gpu_info),
      initialized_(false) {
    metal_impl_ = std::make_unique<metal::MFPMetal>();
}

bool MetalStrategy::initialize() {
    if (initialized_) {
        return true;
    }
    
    // Initialize Metal implementation
    if (!metal_impl_->initialize(gpu_info_.device_id)) {
        return false;
    }
    
    initialized_ = true;
    return true;
}

bool MetalStrategy::isAvailable() const {
    return initialized_ && metal_impl_->isAvailable();
}

bool MetalStrategy::runMFP(MFPMethod method, const std::string& number, std::vector<std::string>& factors) {
    if (!initialized_ || !metal_impl_->isAvailable()) {
        return false;
    }
    
    switch (method) {
        case MFPMethod::METHOD_1:
            return metal_impl_->runMethod1(number, factors);
            
        case MFPMethod::METHOD_2:
            return metal_impl_->runMethod2(number, factors);
            
        case MFPMethod::METHOD_3:
            return metal_impl_->runMethod3(number, factors);
            
        case MFPMethod::AUTO:
            // For AUTO, choose the best method based on number size
            if (number.size() < 100) {
                // For small numbers, use Method 1
                return metal_impl_->runMethod1(number, factors);
            } else if (number.size() < 1000) {
                // For medium numbers, use Method 2
                return metal_impl_->runMethod2(number, factors);
            } else {
                // For large numbers, use Method 3
                return metal_impl_->runMethod3(number, factors);
            }
    }
    
    return false;
}

bool MetalStrategy::isPrime(const std::string& number) {
    if (!initialized_ || !metal_impl_->isAvailable()) {
        return false;
    }
    
    return metal_impl_->isPrime(number);
}

std::string MetalStrategy::findNextPrime(const std::string& number) {
    if (!initialized_ || !metal_impl_->isAvailable()) {
        return "";
    }
    
    return metal_impl_->findNextPrime(number);
}

bool MetalStrategy::findPrimeFactors(const std::string& number, std::vector<std::string>& factors) {
    if (!initialized_ || !metal_impl_->isAvailable()) {
        return false;
    }
    
    return metal_impl_->findPrimeFactors(number, factors);
}

std::string MetalStrategy::getPerformanceMetrics() const {
    if (!initialized_ || !metal_impl_->isAvailable()) {
        return "No performance metrics available for Metal strategy";
    }
    
    return metal_impl_->getPerformanceMetrics();
}

std::string MetalStrategy::getDeviceInfo() const {
    std::stringstream ss;
    ss << "Metal GPU: " << gpu_info_.name << "\n";
    ss << "  Vendor: " << system::GPUVendorToString(gpu_info_.vendor) << "\n";
    ss << "  Architecture: " << system::GPUArchitectureToString(gpu_info_.architecture) << "\n";
    
    if (!gpu_info_.api_support.empty()) {
        ss << "  API Support: ";
        for (size_t i = 0; i < gpu_info_.api_support.size(); ++i) {
            if (i > 0) {
                ss << ", ";
            }
            ss << system::GPUAPISupportToString(gpu_info_.api_support[i]);
        }
        ss << "\n";
    }
    
    ss << "  Memory: " << (gpu_info_.memory_info.total_memory_bytes / (1024 * 1024 * 1024)) << " GB\n";
    
    if (gpu_info_.memory_info.has_unified_memory) {
        ss << "  Unified Memory: Yes\n";
    } else {
        ss << "  Unified Memory: No\n";
    }
    
    ss << "  Max Threads Per Threadgroup: " << gpu_info_.compute_info.max_threads_per_threadgroup << "\n";
    
    if (gpu_info_.is_integrated) {
        ss << "  Type: Integrated\n";
    } else {
        ss << "  Type: Discrete\n";
    }
    
    return ss.str();
}

double MetalStrategy::runBenchmark() {
    if (!initialized_ || !metal_impl_->isAvailable()) {
        return 0.0;
    }
    
    // Run a simple benchmark to measure Metal performance
    const int iterations = 5;
    double total_score = 0.0;
    
    for (int i = 0; i < iterations; ++i) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Perform a compute-intensive task on GPU
        // TODO: Implement a more realistic benchmark
        std::vector<std::string> factors;
        std::string test_number = "1234567890123456789012345678901234567890";
        metal_impl_->runMethod3(test_number, factors);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        
        // Calculate score (higher is better)
        double score = 10000.0 / duration;
        total_score += score;
    }
    
    return total_score / iterations;
}

std::string MetalStrategy::getName() const {
    return "Metal";
}

//=============================================================================
// Hybrid Strategy Implementation
//=============================================================================

HybridStrategy::HybridStrategy(std::shared_ptr<CPUStrategy> cpu_strategy, 
                             std::shared_ptr<ExecutionStrategy> gpu_strategy)
    : cpu_strategy_(cpu_strategy),
      gpu_strategy_(gpu_strategy),
      initialized_(false),
      cpu_workload_ratio_(0.3),
      gpu_workload_ratio_(0.7) {
}

bool HybridStrategy::initialize() {
    if (initialized_) {
        return true;
    }
    
    // Initialize both CPU and GPU strategies
    if (!cpu_strategy_->initialize() || !gpu_strategy_->initialize()) {
        return false;
    }
    
    // Optimize work distribution between CPU and GPU
    optimizeWorkDistribution();
    
    initialized_ = true;
    return true;
}

bool HybridStrategy::isAvailable() const {
    return initialized_ && cpu_strategy_->isAvailable() && gpu_strategy_->isAvailable();
}

bool HybridStrategy::runMFP(MFPMethod method, const std::string& number, std::vector<std::string>& factors) {
    if (!initialized_ || !cpu_strategy_->isAvailable() || !gpu_strategy_->isAvailable()) {
        return false;
    }
    
    // For hybrid strategy, we'll split the work between CPU and GPU
    // This is a simplified implementation; a real implementation would
    // divide the work more intelligently
    
    // For now, we'll just use the GPU strategy for simplicity
    return gpu_strategy_->runMFP(method, number, factors);
}

bool HybridStrategy::isPrime(const std::string& number) {
    if (!initialized_ || !cpu_strategy_->isAvailable() || !gpu_strategy_->isAvailable()) {
        return false;
    }
    
    // For isPrime, we'll use the GPU strategy as it's likely faster
    return gpu_strategy_->isPrime(number);
}

std::string HybridStrategy::findNextPrime(const std::string& number) {
    if (!initialized_ || !cpu_strategy_->isAvailable() || !gpu_strategy_->isAvailable()) {
        return "";
    }
    
    // For findNextPrime, we'll use the GPU strategy as it's likely faster
    return gpu_strategy_->findNextPrime(number);
}

bool HybridStrategy::findPrimeFactors(const std::string& number, std::vector<std::string>& factors) {
    if (!initialized_ || !cpu_strategy_->isAvailable() || !gpu_strategy_->isAvailable()) {
        return false;
    }
    
    // For findPrimeFactors, we'll use the GPU strategy as it's likely faster
    return gpu_strategy_->findPrimeFactors(number, factors);
}

std::string HybridStrategy::getPerformanceMetrics() const {
    if (!initialized_ || !cpu_strategy_->isAvailable() || !gpu_strategy_->isAvailable()) {
        return "No performance metrics available for Hybrid strategy";
    }
    
    std::stringstream ss;
    ss << "Hybrid Strategy Performance Metrics:\n";
    ss << "CPU Workload Ratio: " << (cpu_workload_ratio_ * 100.0) << "%\n";
    ss << "GPU Workload Ratio: " << (gpu_workload_ratio_ * 100.0) << "%\n\n";
    
    ss << "CPU Strategy Metrics:\n";
    ss << cpu_strategy_->getPerformanceMetrics() << "\n";
    
    ss << "GPU Strategy Metrics:\n";
    ss << gpu_strategy_->getPerformanceMetrics();
    
    return ss.str();
}

std::string HybridStrategy::getDeviceInfo() const {
    std::stringstream ss;
    ss << "Hybrid Strategy Device Information:\n";
    ss << "CPU Device:\n";
    ss << cpu_strategy_->getDeviceInfo() << "\n";
    
    ss << "GPU Device:\n";
    ss << gpu_strategy_->getDeviceInfo();
    
    return ss.str();
}

double HybridStrategy::runBenchmark() {
    if (!initialized_ || !cpu_strategy_->isAvailable() || !gpu_strategy_->isAvailable()) {
        return 0.0;
    }
    
    // Run benchmarks on both CPU and GPU
    double cpu_score = cpu_strategy_->runBenchmark();
    double gpu_score = gpu_strategy_->runBenchmark();
    
    // Calculate weighted score based on workload ratios
    return (cpu_score * cpu_workload_ratio_) + (gpu_score * gpu_workload_ratio_);
}

std::string HybridStrategy::getName() const {
    return "Hybrid (" + cpu_strategy_->getName() + " + " + gpu_strategy_->getName() + ")";
}

void HybridStrategy::optimizeWorkDistribution() {
    // Run benchmarks on both CPU and GPU
    double cpu_score = cpu_strategy_->runBenchmark();
    double gpu_score = gpu_strategy_->runBenchmark();
    
    // Calculate workload ratios based on relative performance
    double total_score = cpu_score + gpu_score;
    
    if (total_score > 0) {
        cpu_workload_ratio_ = cpu_score / total_score;
        gpu_workload_ratio_ = gpu_score / total_score;
    } else {
        // Default to 30% CPU, 70% GPU if benchmarks fail
        cpu_workload_ratio_ = 0.3;
        gpu_workload_ratio_ = 0.7;
    }
}

//=============================================================================
// Resource Manager Implementation
//=============================================================================

ResourceManager::ResourceManager()
    : allocation_mode_(AllocationMode::AUTO),
      mfp_method_(MFPMethod::AUTO),
      performance_logging_enabled_(false) {
}

ResourceManager::~ResourceManager() {
    releaseResources();
}

bool ResourceManager::initialize() {
    // Detect system capabilities
    if (!detectSystemCapabilities()) {
        return false;
    }
    
    return true;
}

void ResourceManager::setAllocationMode(AllocationMode mode) {
    allocation_mode_ = mode;
    
    // If we already have a strategy, we need to release it and create a new one
    if (current_strategy_) {
        releaseResources();
    }
}

AllocationMode ResourceManager::getAllocationMode() const {
    return allocation_mode_;
}

void ResourceManager::setMFPMethod(MFPMethod method) {
    mfp_method_ = method;
}

MFPMethod ResourceManager::getMFPMethod() const {
    return mfp_method_;
}

void ResourceManager::setPerformanceLogging(bool enable) {
    performance_logging_enabled_ = enable;
    
    // Update performance logging for current strategy
    if (current_strategy_) {
        if (cpu_strategy_) {
            // TODO: Set performance logging for CPU strategy
        }
        
        if (cuda_strategy_) {
            // TODO: Set performance logging for CUDA strategy
        }
        
        if (metal_strategy_) {
            // TODO: Set performance logging for Metal strategy
        }
    }
}

bool ResourceManager::getPerformanceLogging() const {
    return performance_logging_enabled_;
}

AllocationResult ResourceManager::allocateResources(size_t required_memory_bytes) {
    AllocationResult result;
    result.success = false;
    
    // Select the best strategy based on allocation mode and system capabilities
    std::shared_ptr<ExecutionStrategy> strategy;
    
    if (allocation_mode_ == AllocationMode::AUTO) {
        strategy = selectBestStrategy(required_memory_bytes);
    } else {
        strategy = createStrategyForMode(allocation_mode_);
    }
    
    if (!strategy) {
        result.error_message = "Failed to create execution strategy";
        return result;
    }
    
    // Initialize the strategy
    if (!strategy->initialize()) {
        result.error_message = "Failed to initialize execution strategy";
        return result;
    }
    
    // Set the current strategy
    current_strategy_ = strategy;
    
    // Fill in allocation result
    result.success = true;
    result.device_name = strategy->getDeviceInfo();
    result.device_type = strategy->getName();
    
    // TODO: Fill in cores_or_compute_units and memory_allocated_bytes
    
    return result;
}

void ResourceManager::releaseResources() {
    // Release current strategy
    current_strategy_.reset();
    
    // Release all strategies
    cpu_strategy_.reset();
    cuda_strategy_.reset();
    metal_strategy_.reset();
    hybrid_strategy_.reset();
}

bool ResourceManager::runMFP(const std::string& number, std::vector<std::string>& factors) {
    if (!current_strategy_) {
        // Allocate resources if not already allocated
        AllocationResult result = allocateResources();
        if (!result.success) {
            return false;
        }
    }
    
    // Run MFP with the current strategy
    return current_strategy_->runMFP(mfp_method_, number, factors);
}

bool ResourceManager::isPrime(const std::string& number) {
    if (!current_strategy_) {
        // Allocate resources if not already allocated
        AllocationResult result = allocateResources();
        if (!result.success) {
            return false;
        }
    }
    
    // Check if number is prime with the current strategy
    return current_strategy_->isPrime(number);
}

std::string ResourceManager::findNextPrime(const std::string& number) {
    if (!current_strategy_) {
        // Allocate resources if not already allocated
        AllocationResult result = allocateResources();
        if (!result.success) {
            return "";
        }
    }
    
    // Find next prime with the current strategy
    return current_strategy_->findNextPrime(number);
}

bool ResourceManager::findPrimeFactors(const std::string& number, std::vector<std::string>& factors) {
    if (!current_strategy_) {
        // Allocate resources if not already allocated
        AllocationResult result = allocateResources();
        if (!result.success) {
            return false;
        }
    }
    
    // Find prime factors with the current strategy
    return current_strategy_->findPrimeFactors(number, factors);
}

std::string ResourceManager::getPerformanceMetrics() const {
    if (!current_strategy_) {
        return "No performance metrics available (no active strategy)";
    }
    
    return current_strategy_->getPerformanceMetrics();
}

std::string ResourceManager::getSystemInfo() const {
    std::stringstream ss;
    
    // CPU information
    ss << "CPU Information:\n";
    ss << "  Model: " << cpu_info_.model_name << "\n";
    ss << "  Architecture: " << cpu_info_.architecture << "\n";
    ss << "  Physical Cores: " << cpu_info_.physical_cores << "\n";
    ss << "  Logical Cores: " << cpu_info_.logical_cores << "\n";
    ss << "  Base Frequency: " << cpu_info_.base_frequency_mhz << " MHz\n";
    
    if (!cpu_info_.features.empty()) {
        ss << "  Features: ";
        for (size_t i = 0; i < cpu_info_.features.size(); ++i) {
            if (i > 0) {
                ss << ", ";
            }
            ss << cpu_info_.features[i];
        }
        ss << "\n";
    }
    
    ss << "\n";
    
    // Memory information
    ss << "Memory Information:\n";
    ss << "  Total Memory: " << (memory_info_.total_memory_bytes / (1024 * 1024 * 1024)) << " GB\n";
    ss << "  Available Memory: " << (memory_info_.available_memory_bytes / (1024 * 1024 * 1024)) << " GB\n";
    
    if (memory_info_.memory_type != system::MemoryType::UNKNOWN) {
        ss << "  Memory Type: " << system::MemoryTypeToString(memory_info_.memory_type) << "\n";
    }
    
    if (memory_info_.memory_speed_mhz > 0) {
        ss << "  Memory Speed: " << memory_info_.memory_speed_mhz << " MHz\n";
    }
    
    ss << "\n";
    
    // GPU information
    ss << "GPU Information:\n";
    
    if (gpu_info_.empty()) {
        ss << "  No GPUs detected\n";
    } else {
        for (size_t i = 0; i < gpu_info_.size(); ++i) {
            const auto& gpu = gpu_info_[i];
            
            ss << "  GPU " << i << ": " << gpu.name << "\n";
            ss << "    Vendor: " << system::GPUVendorToString(gpu.vendor) << "\n";
            ss << "    Architecture: " << system::GPUArchitectureToString(gpu.architecture) << "\n";
            
            if (!gpu.api_support.empty()) {
                ss << "    API Support: ";
                for (size_t j = 0; j < gpu.api_support.size(); ++j) {
                    if (j > 0) {
                        ss << ", ";
                    }
                    ss << system::GPUAPISupportToString(gpu.api_support[j]);
                }
                ss << "\n";
            }
            
            ss << "    Memory: " << (gpu.memory_info.total_memory_bytes / (1024 * 1024 * 1024)) << " GB\n";
            
            if (gpu.vendor == system::GPUVendor::NVIDIA) {
                ss << "    CUDA Cores: " << gpu.compute_info.cuda_cores << "\n";
                ss << "    Compute Capability: " << gpu.compute_info.cuda_compute_capability << "\n";
            }
            
            if (gpu.is_integrated) {
                ss << "    Type: Integrated\n";
            } else {
                ss << "    Type: Discrete\n";
            }
            
            ss << "\n";
        }
    }
    
    // Current allocation mode
    ss << "Current Allocation Mode: ";
    switch (allocation_mode_) {
        case AllocationMode::AUTO:
            ss << "AUTO (Automatic Selection)";
            break;
            
        case AllocationMode::CPU_ONLY:
            ss << "CPU_ONLY";
            break;
            
        case AllocationMode::GPU_ONLY:
            ss << "GPU_ONLY";
            break;
            
        case AllocationMode::CUDA_ONLY:
            ss << "CUDA_ONLY";
            break;
            
        case AllocationMode::METAL_ONLY:
            ss << "METAL_ONLY";
            break;
            
        case AllocationMode::HYBRID:
            ss << "HYBRID";
            break;
    }
    ss << "\n";
    
    // Current MFP method
    ss << "Current MFP Method: ";
    switch (mfp_method_) {
        case MFPMethod::AUTO:
            ss << "AUTO (Automatic Selection)";
            break;
            
        case MFPMethod::METHOD_1:
            ss << "METHOD_1 (Expanded q Factorization)";
            break;
            
        case MFPMethod::METHOD_2:
            ss << "METHOD_2 (Ultrafast with Structural Filter)";
            break;
            
        case MFPMethod::METHOD_3:
            ss << "METHOD_3 (Parallelized with Dynamic Blocks)";
            break;
    }
    ss << "\n";
    
    // Performance logging
    ss << "Performance Logging: " << (performance_logging_enabled_ ? "Enabled" : "Disabled") << "\n";
    
    return ss.str();
}

BenchmarkResult ResourceManager::runBenchmark() {
    BenchmarkResult result;
    result.cpu_score = 0.0;
    result.cuda_score = 0.0;
    result.metal_score = 0.0;
    
    // Initialize CPU strategy if not already initialized
    if (!cpu_strategy_) {
        cpu_strategy_ = std::make_shared<CPUStrategy>(cpu_info_, memory_info_);
        cpu_strategy_->initialize();
    }
    
    // Run CPU benchmark
    if (cpu_strategy_->isAvailable()) {
        result.cpu_score = cpu_strategy_->runBenchmark();
    }
    
    // Initialize CUDA strategy if not already initialized
    if (!cuda_strategy_ && !gpu_info_.empty()) {
        // Find NVIDIA GPU
        for (const auto& gpu : gpu_info_) {
            if (gpu.vendor == system::GPUVendor::NVIDIA) {
                cuda_strategy_ = std::make_shared<CUDAStrategy>(gpu);
                cuda_strategy_->initialize();
                break;
            }
        }
    }
    
    // Run CUDA benchmark
    if (cuda_strategy_ && cuda_strategy_->isAvailable()) {
        result.cuda_score = cuda_strategy_->runBenchmark();
    }
    
    // Initialize Metal strategy if not already initialized
    if (!metal_strategy_ && !gpu_info_.empty()) {
        // Find Apple GPU
        for (const auto& gpu : gpu_info_) {
            if (gpu.vendor == system::GPUVendor::APPLE) {
                metal_strategy_ = std::make_shared<MetalStrategy>(gpu);
                metal_strategy_->initialize();
                break;
            }
        }
    }
    
    // Run Metal benchmark
    if (metal_strategy_ && metal_strategy_->isAvailable()) {
        result.metal_score = metal_strategy_->runBenchmark();
    }
    
    // Determine best device
    if (result.cpu_score >= result.cuda_score && result.cpu_score >= result.metal_score) {
        result.best_device = "CPU";
    } else if (result.cuda_score >= result.cpu_score && result.cuda_score >= result.metal_score) {
        result.best_device = "CUDA";
    } else {
        result.best_device = "Metal";
    }
    
    // Generate details
    std::stringstream ss;
    ss << "Benchmark Results:\n";
    ss << "  CPU Score: " << std::fixed << std::setprecision(2) << result.cpu_score << "\n";
    ss << "  CUDA Score: " << std::fixed << std::setprecision(2) << result.cuda_score << "\n";
    ss << "  Metal Score: " << std::fixed << std::setprecision(2) << result.metal_score << "\n";
    ss << "  Best Device: " << result.best_device << "\n";
    
    result.details = ss.str();
    
    return result;
}

std::shared_ptr<ExecutionStrategy> ResourceManager::selectBestStrategy(size_t required_memory_bytes) {
    // Run benchmarks to determine the best strategy
    BenchmarkResult benchmark = runBenchmark();
    
    // Select the best strategy based on benchmark results
    if (benchmark.best_device == "CPU") {
        return cpu_strategy_;
    } else if (benchmark.best_device == "CUDA") {
        return cuda_strategy_;
    } else if (benchmark.best_device == "Metal") {
        return metal_strategy_;
    } else {
        // Default to CPU if no clear winner
        return cpu_strategy_;
    }
}

std::shared_ptr<ExecutionStrategy> ResourceManager::createStrategyForMode(AllocationMode mode) {
    switch (mode) {
        case AllocationMode::CPU_ONLY:
            if (!cpu_strategy_) {
                cpu_strategy_ = std::make_shared<CPUStrategy>(cpu_info_, memory_info_);
            }
            return cpu_strategy_;
            
        case AllocationMode::GPU_ONLY:
            // Try CUDA first, then Metal
            if (cuda_strategy_ && cuda_strategy_->isAvailable()) {
                return cuda_strategy_;
            } else if (metal_strategy_ && metal_strategy_->isAvailable()) {
                return metal_strategy_;
            } else {
                // No GPU available, fall back to CPU
                if (!cpu_strategy_) {
                    cpu_strategy_ = std::make_shared<CPUStrategy>(cpu_info_, memory_info_);
                }
                return cpu_strategy_;
            }
            
        case AllocationMode::CUDA_ONLY:
            if (cuda_strategy_ && cuda_strategy_->isAvailable()) {
                return cuda_strategy_;
            } else {
                // CUDA not available, fall back to CPU
                if (!cpu_strategy_) {
                    cpu_strategy_ = std::make_shared<CPUStrategy>(cpu_info_, memory_info_);
                }
                return cpu_strategy_;
            }
            
        case AllocationMode::METAL_ONLY:
            if (metal_strategy_ && metal_strategy_->isAvailable()) {
                return metal_strategy_;
            } else {
                // Metal not available, fall back to CPU
                if (!cpu_strategy_) {
                    cpu_strategy_ = std::make_shared<CPUStrategy>(cpu_info_, memory_info_);
                }
                return cpu_strategy_;
            }
            
        case AllocationMode::HYBRID:
            // Create hybrid strategy with CPU and the best GPU
            if (!cpu_strategy_) {
                cpu_strategy_ = std::make_shared<CPUStrategy>(cpu_info_, memory_info_);
            }
            
            if (cuda_strategy_ && cuda_strategy_->isAvailable()) {
                if (!hybrid_strategy_) {
                    hybrid_strategy_ = std::make_shared<HybridStrategy>(cpu_strategy_, cuda_strategy_);
                }
                return hybrid_strategy_;
            } else if (metal_strategy_ && metal_strategy_->isAvailable()) {
                if (!hybrid_strategy_) {
                    hybrid_strategy_ = std::make_shared<HybridStrategy>(cpu_strategy_, metal_strategy_);
                }
                return hybrid_strategy_;
            } else {
                // No GPU available, fall back to CPU
                return cpu_strategy_;
            }
            
        case AllocationMode::AUTO:
        default:
            // This should be handled by selectBestStrategy
            return selectBestStrategy(0);
    }
}

MFPMethod ResourceManager::selectBestMethod(const std::string& number) {
    // Select the best method based on number size and available resources
    size_t number_size = number.size();
    
    if (number_size < 100) {
        // For small numbers, use Method 1
        return MFPMethod::METHOD_1;
    } else if (number_size < 1000) {
        // For medium numbers, use Method 2
        return MFPMethod::METHOD_2;
    } else {
        // For large numbers, use Method 3
        return MFPMethod::METHOD_3;
    }
}

bool ResourceManager::detectSystemCapabilities() {
    // Detect CPU capabilities
    system::CPUDetector cpu_detector;
    if (!cpu_detector.detect(cpu_info_)) {
        return false;
    }
    
    // Detect memory capabilities
    system::MemoryStorageDetector memory_detector;
    if (!memory_detector.detectMemory(memory_info_)) {
        return false;
    }
    
    // Detect GPU capabilities
    system::GPUDetector gpu_detector;
    if (!gpu_detector.detect(gpu_info_)) {
        return false;
    }
    
    // Initialize strategies based on detected capabilities
    
    // CPU strategy
    cpu_strategy_ = std::make_shared<CPUStrategy>(cpu_info_, memory_info_);
    
    // CUDA strategy (if NVIDIA GPU is available)
    for (const auto& gpu : gpu_info_) {
        if (gpu.vendor == system::GPUVendor::NVIDIA) {
            cuda_strategy_ = std::make_shared<CUDAStrategy>(gpu);
            break;
        }
    }
    
    // Metal strategy (if Apple GPU is available)
    for (const auto& gpu : gpu_info_) {
        if (gpu.vendor == system::GPUVendor::APPLE) {
            metal_strategy_ = std::make_shared<MetalStrategy>(gpu);
            break;
        }
    }
    
    return true;
}

} // namespace resource
} // namespace mfp
