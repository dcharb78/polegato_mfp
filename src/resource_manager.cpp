#include "resource_manager.h"
#include "gpu/cuda_accelerator.h"
#include "gpu/metal_accelerator.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <thread>
#include <chrono>
#include <random>
#include <cmath>

namespace mfp {

// Static instance for global access
static ResourceManager s_instance;

ResourceManager::ResourceManager() 
    : m_strategy(ExecutionStrategy::AUTO), 
      m_mode(AllocationMode::AUTO) {
}

ResourceManager::~ResourceManager() {
    // Cleanup happens in member destructors
}

bool ResourceManager::initialize() {
    // Detect hardware capabilities
    detectHardware();
    
    // Initialize accelerators if available
    if (isCUDAAvailable()) {
        m_cuda_accelerator = std::make_shared<CUDAAccelerator>();
        if (!m_cuda_accelerator->initialize(getBestGPU())) {
            m_cuda_accelerator.reset();
        }
    }
    
    if (isMetalAvailable()) {
        m_metal_accelerator = std::make_shared<MetalAccelerator>();
        if (!m_metal_accelerator->initialize(getBestGPU())) {
            m_metal_accelerator.reset();
        }
    }
    
    // Determine optimal strategy based on available hardware
    determineOptimalStrategy();
    
    return true;
}

const CPUInfo& ResourceManager::getCPUInfo() const {
    return m_cpu_detector.getCPUInfo();
}

const MemoryInfo& ResourceManager::getMemoryInfo() const {
    return m_memory_storage_detector.getMemoryInfo();
}

const StorageInfo& ResourceManager::getStorageInfo() const {
    return m_memory_storage_detector.getStorageInfo();
}

const std::vector<GPUInfo>& ResourceManager::getGPUs() const {
    return m_gpu_detector.getGPUs();
}

GPUInfo ResourceManager::getBestGPU() const {
    return m_gpu_detector.findBestComputeGPU();
}

void ResourceManager::setExecutionStrategy(ExecutionStrategy strategy) {
    m_strategy = strategy;
    
    // If strategy is AUTO, determine the optimal strategy
    if (m_strategy == ExecutionStrategy::AUTO) {
        determineOptimalStrategy();
    }
}

ExecutionStrategy ResourceManager::getExecutionStrategy() const {
    return m_strategy;
}

void ResourceManager::setAllocationMode(AllocationMode mode) {
    m_mode = mode;
}

AllocationMode ResourceManager::getAllocationMode() const {
    return m_mode;
}

int ResourceManager::getOptimalThreadCount() const {
    const CPUInfo& cpu_info = getCPUInfo();
    
    // Start with logical core count
    int thread_count = cpu_info.logical_cores;
    
    // Adjust based on allocation mode
    switch (m_mode) {
        case AllocationMode::PERFORMANCE:
            // Use all available cores
            break;
            
        case AllocationMode::MEMORY:
            // Use fewer cores to reduce memory usage
            thread_count = std::max(1, thread_count / 2);
            break;
            
        case AllocationMode::BALANCED:
            // Use a balanced number of cores
            thread_count = std::max(1, thread_count * 3 / 4);
            break;
            
        case AllocationMode::AUTO:
            // Adjust based on CPU architecture and workload
            if (cpu_info.has_hyperthreading) {
                // For hyperthreaded CPUs, using physical core count often gives better performance
                thread_count = cpu_info.physical_cores;
            }
            break;
    }
    
    // Ensure we have at least one thread
    return std::max(1, thread_count);
}

size_t ResourceManager::getOptimalBlockSize() const {
    const CPUInfo& cpu_info = getCPUInfo();
    
    // Base block size on L1 cache size if available
    if (cpu_info.l1_cache_size > 0) {
        return cpu_info.l1_cache_size / 2;
    }
    
    // Default block size
    return 4096;
}

size_t ResourceManager::getOptimalMemoryLimit() const {
    const MemoryInfo& memory_info = getMemoryInfo();
    
    // Calculate memory limit based on allocation mode
    double memory_percentage = 0.0;
    
    switch (m_mode) {
        case AllocationMode::PERFORMANCE:
            // Use up to 80% of available memory
            memory_percentage = 0.8;
            break;
            
        case AllocationMode::MEMORY:
            // Use up to 40% of available memory
            memory_percentage = 0.4;
            break;
            
        case AllocationMode::BALANCED:
            // Use up to 60% of available memory
            memory_percentage = 0.6;
            break;
            
        case AllocationMode::AUTO:
            // Adjust based on available memory
            if (memory_info.total_physical_memory > 16ULL * 1024 * 1024 * 1024) {
                // High memory system (>16GB)
                memory_percentage = 0.7;
            } else if (memory_info.total_physical_memory > 8ULL * 1024 * 1024 * 1024) {
                // Medium memory system (8-16GB)
                memory_percentage = 0.6;
            } else if (memory_info.total_physical_memory > 4ULL * 1024 * 1024 * 1024) {
                // Low memory system (4-8GB)
                memory_percentage = 0.5;
            } else {
                // Very low memory system (<4GB)
                memory_percentage = 0.4;
            }
            break;
    }
    
    // Calculate memory limit
    return static_cast<size_t>(memory_info.available_physical_memory * memory_percentage);
}

bool ResourceManager::isCUDAAvailable() const {
    return m_gpu_detector.hasCUDAGPU();
}

bool ResourceManager::isMetalAvailable() const {
    return m_gpu_detector.hasMetalGPU();
}

std::shared_ptr<CUDAAccelerator> ResourceManager::getCUDAAccelerator() {
    return m_cuda_accelerator;
}

std::shared_ptr<MetalAccelerator> ResourceManager::getMetalAccelerator() {
    return m_metal_accelerator;
}

std::unique_ptr<MFPBase> ResourceManager::createMFP(int method_number) {
    // Create MFP implementation based on current strategy
    switch (m_strategy) {
        case ExecutionStrategy::CPU_ONLY:
            // Create CPU-only implementation
            return createMFPMethod(method_number);
            
        case ExecutionStrategy::CUDA_GPU:
            // Create CUDA implementation if available
            if (m_cuda_accelerator) {
                return createCUDAMFP(method_number);
            }
            // Fall back to CPU if CUDA is not available
            return createMFPMethod(method_number);
            
        case ExecutionStrategy::METAL_GPU:
            // Create Metal implementation if available
            if (m_metal_accelerator) {
                return createMetalMFP(method_number);
            }
            // Fall back to CPU if Metal is not available
            return createMFPMethod(method_number);
            
        case ExecutionStrategy::HYBRID:
            // Create hybrid implementation (not implemented yet)
            // For now, fall back to the best available option
            if (m_cuda_accelerator) {
                return createCUDAMFP(method_number);
            } else if (m_metal_accelerator) {
                return createMetalMFP(method_number);
            } else {
                return createMFPMethod(method_number);
            }
            
        case ExecutionStrategy::AUTO:
        default:
            // Use the strategy determined by determineOptimalStrategy()
            return createMFP(method_number);
    }
}

void ResourceManager::runBenchmark() {
    // Create a large number for benchmarking
    mpz_t number;
    mpz_init(number);
    
    // Generate a random 1000-bit number
    gmp_randstate_t state;
    gmp_randinit_default(state);
    gmp_randseed_ui(state, static_cast<unsigned long>(std::time(nullptr)));
    mpz_urandomb(number, state, 1000);
    gmp_randclear(state);
    
    // Run benchmark to determine best strategy
    ExecutionStrategy best_strategy = benchmarkStrategies(number);
    
    // Set the strategy
    m_strategy = best_strategy;
    
    // Clean up
    mpz_clear(number);
}

std::string ResourceManager::getSystemSummary() const {
    std::stringstream ss;
    
    ss << "System Summary:" << std::endl;
    ss << "================" << std::endl;
    
    // CPU information
    const CPUInfo& cpu_info = getCPUInfo();
    ss << "CPU:" << std::endl;
    ss << "  Model: " << cpu_info.model_name << std::endl;
    ss << "  Architecture: " << cpu_info.architecture << std::endl;
    ss << "  Physical cores: " << cpu_info.physical_cores << std::endl;
    ss << "  Logical cores: " << cpu_info.logical_cores << std::endl;
    ss << "  Hyperthreading: " << (cpu_info.has_hyperthreading ? "Yes" : "No") << std::endl;
    ss << "  Features: ";
    if (cpu_info.has_avx) ss << "AVX ";
    if (cpu_info.has_avx2) ss << "AVX2 ";
    if (cpu_info.has_avx512) ss << "AVX512 ";
    if (cpu_info.has_sse4) ss << "SSE4 ";
    ss << std::endl;
    
    // Memory information
    const MemoryInfo& memory_info = getMemoryInfo();
    ss << "Memory:" << std::endl;
    ss << "  Total physical memory: " << (memory_info.total_physical_memory / (1024 * 1024 * 1024)) << " GB" << std::endl;
    ss << "  Available physical memory: " << (memory_info.available_physical_memory / (1024 * 1024 * 1024)) << " GB" << std::endl;
    ss << "  Total virtual memory: " << (memory_info.total_virtual_memory / (1024 * 1024 * 1024)) << " GB" << std::endl;
    ss << "  Available virtual memory: " << (memory_info.available_virtual_memory / (1024 * 1024 * 1024)) << " GB" << std::endl;
    
    // Storage information
    const StorageInfo& storage_info = getStorageInfo();
    ss << "Storage:" << std::endl;
    ss << "  Primary storage type: " << storage_info.primary_storage_type << std::endl;
    ss << "  Primary storage capacity: " << (storage_info.primary_storage_capacity / (1024 * 1024 * 1024)) << " GB" << std::endl;
    ss << "  Primary storage available: " << (storage_info.primary_storage_available / (1024 * 1024 * 1024)) << " GB" << std::endl;
    
    // GPU information
    const std::vector<GPUInfo>& gpus = getGPUs();
    ss << "GPUs:" << std::endl;
    if (gpus.empty()) {
        ss << "  No GPUs detected" << std::endl;
    } else {
        for (size_t i = 0; i < gpus.size(); i++) {
            ss << "  GPU " << i << ":" << std::endl;
            ss << "    Name: " << gpus[i].getName() << std::endl;
            ss << "    Vendor: ";
            switch (gpus[i].getVendor()) {
                case GPUVendor::NVIDIA: ss << "NVIDIA"; break;
                case GPUVendor::AMD: ss << "AMD"; break;
                case GPUVendor::INTEL: ss << "Intel"; break;
                case GPUVendor::APPLE: ss << "Apple"; break;
                default: ss << "Unknown"; break;
            }
            ss << std::endl;
            
            ss << "    APIs: ";
            const GPUAPIs& apis = gpus[i].getAPIs();
            if (apis.supports_cuda) ss << "CUDA ";
            if (apis.supports_opencl) ss << "OpenCL ";
            if (apis.supports_metal) ss << "Metal ";
            if (apis.supports_directx) ss << "DirectX ";
            if (apis.supports_vulkan) ss << "Vulkan ";
            ss << std::endl;
            
            ss << "    Memory: " << (gpus[i].getMemory().total_memory_bytes / (1024 * 1024 * 1024)) << " GB" << std::endl;
        }
    }
    
    // Current configuration
    ss << "Configuration:" << std::endl;
    ss << "  Execution strategy: ";
    switch (m_strategy) {
        case ExecutionStrategy::AUTO: ss << "Auto"; break;
        case ExecutionStrategy::CPU_ONLY: ss << "CPU only"; break;
        case ExecutionStrategy::CUDA_GPU: ss << "CUDA GPU"; break;
        case ExecutionStrategy::METAL_GPU: ss << "Metal GPU"; break;
        case ExecutionStrategy::HYBRID: ss << "Hybrid"; break;
    }
    ss << std::endl;
    
    ss << "  Allocation mode: ";
    switch (m_mode) {
        case AllocationMode::AUTO: ss << "Auto"; break;
        case AllocationMode::PERFORMANCE: ss << "Performance"; break;
        case AllocationMode::MEMORY: ss << "Memory"; break;
        case AllocationMode::BALANCED: ss << "Balanced"; break;
    }
    ss << std::endl;
    
    ss << "  Optimal thread count: " << getOptimalThreadCount() << std::endl;
    ss << "  Optimal block size: " << getOptimalBlockSize() << " bytes" << std::endl;
    ss << "  Optimal memory limit: " << (getOptimalMemoryLimit() / (1024 * 1024)) << " MB" << std::endl;
    
    return ss.str();
}

void ResourceManager::detectHardware() {
    // Detect CPU capabilities
    m_cpu_detector.detect();
    
    // Detect memory and storage capabilities
    m_memory_storage_detector.detect();
    
    // Detect GPU capabilities
    m_gpu_detector.detect();
}

void ResourceManager::determineOptimalStrategy() {
    // Determine the optimal execution strategy based on available hardware
    
    // Check if CUDA is available
    if (isCUDAAvailable() && m_cuda_accelerator) {
        m_strategy = ExecutionStrategy::CUDA_GPU;
        return;
    }
    
    // Check if Metal is available
    if (isMetalAvailable() && m_metal_accelerator) {
        m_strategy = ExecutionStrategy::METAL_GPU;
        return;
    }
    
    // Fall back to CPU only
    m_strategy = ExecutionStrategy::CPU_ONLY;
}

ExecutionStrategy ResourceManager::benchmarkStrategies(const mpz_t number) {
    // Benchmark different strategies and return the fastest one
    
    // Initialize results
    double cpu_time = std::numeric_limits<double>::max();
    double cuda_time = std::numeric_limits<double>::max();
    double metal_time = std::numeric_limits<double>::max();
    
    // Benchmark CPU
    {
        auto start = std::chrono::high_resolution_clock::now();
        
        // Create CPU implementation
        auto mfp = createMFPMethod(1);  // Use method 1 for benchmarking
        
        // Run isPrime operation
        bool is_prime;
        mfp->isPrime(number, is_prime);
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;
        
        cpu_time = elapsed.count();
    }
    
    // Benchmark CUDA if available
    if (m_cuda_accelerator) {
        cuda_time = m_cuda_accelerator->benchmark(number);
    }
    
    // Benchmark Metal if available
    if (m_metal_accelerator) {
        metal_time = m_metal_accelerator->benchmark(number);
    }
    
    // Determine the fastest strategy
    if (cuda_time < cpu_time && cuda_time < metal_time) {
        return ExecutionStrategy::CUDA_GPU;
    } else if (metal_time < cpu_time && metal_time < cuda_time) {
        return ExecutionStrategy::METAL_GPU;
    } else {
        return ExecutionStrategy::CPU_ONLY;
    }
}

// Global resource manager instance
ResourceManager& getResourceManager() {
    return s_instance;
}

} // namespace mfp
