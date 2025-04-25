#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <gmp.h>
#include "mfp_base.h"
#include "hardware/cpu_detector.h"
#include "hardware/memory_storage_detector.h"
#include "hardware/gpu_detector.h"

namespace mfp {

// Forward declarations
class CUDAAccelerator;
class MetalAccelerator;

// Execution strategy types
enum class ExecutionStrategy {
    AUTO,       // Automatically select the best strategy
    CPU_ONLY,   // Use only CPU
    CUDA_GPU,   // Use NVIDIA GPU with CUDA
    METAL_GPU,  // Use Apple GPU with Metal
    HYBRID      // Use both CPU and GPU
};

// Resource allocation mode
enum class AllocationMode {
    AUTO,       // Automatically select the best mode
    PERFORMANCE, // Optimize for performance
    MEMORY,     // Optimize for memory usage
    BALANCED    // Balance performance and memory usage
};

// Resource manager class
class ResourceManager {
public:
    ResourceManager();
    ~ResourceManager();
    
    // Initialize resource manager
    bool initialize();
    
    // Get hardware information
    const CPUInfo& getCPUInfo() const;
    const MemoryInfo& getMemoryInfo() const;
    const StorageInfo& getStorageInfo() const;
    const std::vector<GPUInfo>& getGPUs() const;
    
    // Get best GPU for compute
    GPUInfo getBestGPU() const;
    
    // Set execution strategy
    void setExecutionStrategy(ExecutionStrategy strategy);
    
    // Get current execution strategy
    ExecutionStrategy getExecutionStrategy() const;
    
    // Set allocation mode
    void setAllocationMode(AllocationMode mode);
    
    // Get current allocation mode
    AllocationMode getAllocationMode() const;
    
    // Get optimal thread count for CPU operations
    int getOptimalThreadCount() const;
    
    // Get optimal block size for operations
    size_t getOptimalBlockSize() const;
    
    // Get optimal memory limit
    size_t getOptimalMemoryLimit() const;
    
    // Check if CUDA is available
    bool isCUDAAvailable() const;
    
    // Check if Metal is available
    bool isMetalAvailable() const;
    
    // Get CUDA accelerator
    std::shared_ptr<CUDAAccelerator> getCUDAAccelerator();
    
    // Get Metal accelerator
    std::shared_ptr<MetalAccelerator> getMetalAccelerator();
    
    // Create MFP implementation based on current strategy
    std::unique_ptr<MFPBase> createMFP(int method_number);
    
    // Run benchmark to determine best strategy
    void runBenchmark();
    
    // Get system summary
    std::string getSystemSummary() const;
    
private:
    CPUDetector m_cpu_detector;
    MemoryStorageDetector m_memory_storage_detector;
    GPUDetector m_gpu_detector;
    
    ExecutionStrategy m_strategy;
    AllocationMode m_mode;
    
    std::shared_ptr<CUDAAccelerator> m_cuda_accelerator;
    std::shared_ptr<MetalAccelerator> m_metal_accelerator;
    
    // Helper methods
    void detectHardware();
    void determineOptimalStrategy();
    ExecutionStrategy benchmarkStrategies(const mpz_t number);
};

// Global resource manager instance
ResourceManager& getResourceManager();

} // namespace mfp
