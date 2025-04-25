#include <iostream>
#include <gtest/gtest.h>
#include "mfp_base.h"
#include "mfp_method1.h"
#include "mfp_method2.h"
#include "mfp_method3.h"
#include "resource_manager.h"
#include "configuration_manager.h"
#include "hardware/cpu_detector.h"
#include "hardware/memory_storage_detector.h"
#include "hardware/gpu_detector.h"

namespace mfp {
namespace test {

// Test fixture for hardware detection tests
class HardwareDetectionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize resource manager
        ResourceManager& resource_manager = getResourceManager();
        resource_manager.initialize();
    }
};

// Test CPU detection
TEST_F(HardwareDetectionTest, CPUDetection) {
    ResourceManager& resource_manager = getResourceManager();
    const CPUInfo& cpu_info = resource_manager.getCPUInfo();
    
    // Basic validation
    EXPECT_GT(cpu_info.physical_cores, 0);
    EXPECT_GT(cpu_info.logical_cores, 0);
    EXPECT_GE(cpu_info.logical_cores, cpu_info.physical_cores);
    EXPECT_FALSE(cpu_info.model_name.empty());
    EXPECT_FALSE(cpu_info.architecture.empty());
    
    // Print CPU info for debugging
    std::cout << "CPU Info:" << std::endl;
    std::cout << "  Model: " << cpu_info.model_name << std::endl;
    std::cout << "  Architecture: " << cpu_info.architecture << std::endl;
    std::cout << "  Physical cores: " << cpu_info.physical_cores << std::endl;
    std::cout << "  Logical cores: " << cpu_info.logical_cores << std::endl;
    std::cout << "  Hyperthreading: " << (cpu_info.has_hyperthreading ? "Yes" : "No") << std::endl;
    std::cout << "  Features: ";
    if (cpu_info.has_avx) std::cout << "AVX ";
    if (cpu_info.has_avx2) std::cout << "AVX2 ";
    if (cpu_info.has_avx512) std::cout << "AVX512 ";
    if (cpu_info.has_sse4) std::cout << "SSE4 ";
    std::cout << std::endl;
}

// Test memory detection
TEST_F(HardwareDetectionTest, MemoryDetection) {
    ResourceManager& resource_manager = getResourceManager();
    const MemoryInfo& memory_info = resource_manager.getMemoryInfo();
    
    // Basic validation
    EXPECT_GT(memory_info.total_physical_memory, 0);
    EXPECT_GT(memory_info.available_physical_memory, 0);
    EXPECT_LE(memory_info.available_physical_memory, memory_info.total_physical_memory);
    
    // Print memory info for debugging
    std::cout << "Memory Info:" << std::endl;
    std::cout << "  Total physical memory: " << (memory_info.total_physical_memory / (1024 * 1024 * 1024)) << " GB" << std::endl;
    std::cout << "  Available physical memory: " << (memory_info.available_physical_memory / (1024 * 1024 * 1024)) << " GB" << std::endl;
    std::cout << "  Total virtual memory: " << (memory_info.total_virtual_memory / (1024 * 1024 * 1024)) << " GB" << std::endl;
    std::cout << "  Available virtual memory: " << (memory_info.available_virtual_memory / (1024 * 1024 * 1024)) << " GB" << std::endl;
}

// Test storage detection
TEST_F(HardwareDetectionTest, StorageDetection) {
    ResourceManager& resource_manager = getResourceManager();
    const StorageInfo& storage_info = resource_manager.getStorageInfo();
    
    // Basic validation
    EXPECT_GT(storage_info.primary_storage_capacity, 0);
    EXPECT_GT(storage_info.primary_storage_available, 0);
    EXPECT_LE(storage_info.primary_storage_available, storage_info.primary_storage_capacity);
    EXPECT_FALSE(storage_info.primary_storage_type.empty());
    
    // Print storage info for debugging
    std::cout << "Storage Info:" << std::endl;
    std::cout << "  Primary storage type: " << storage_info.primary_storage_type << std::endl;
    std::cout << "  Primary storage capacity: " << (storage_info.primary_storage_capacity / (1024 * 1024 * 1024)) << " GB" << std::endl;
    std::cout << "  Primary storage available: " << (storage_info.primary_storage_available / (1024 * 1024 * 1024)) << " GB" << std::endl;
}

// Test GPU detection
TEST_F(HardwareDetectionTest, GPUDetection) {
    ResourceManager& resource_manager = getResourceManager();
    const std::vector<GPUInfo>& gpus = resource_manager.getGPUs();
    
    // Print GPU info for debugging
    std::cout << "GPU Info:" << std::endl;
    if (gpus.empty()) {
        std::cout << "  No GPUs detected" << std::endl;
    } else {
        for (size_t i = 0; i < gpus.size(); i++) {
            std::cout << "  GPU " << i << ":" << std::endl;
            std::cout << "    Name: " << gpus[i].getName() << std::endl;
            std::cout << "    Vendor: ";
            switch (gpus[i].getVendor()) {
                case GPUVendor::NVIDIA: std::cout << "NVIDIA"; break;
                case GPUVendor::AMD: std::cout << "AMD"; break;
                case GPUVendor::INTEL: std::cout << "Intel"; break;
                case GPUVendor::APPLE: std::cout << "Apple"; break;
                default: std::cout << "Unknown"; break;
            }
            std::cout << std::endl;
            
            std::cout << "    APIs: ";
            const GPUAPIs& apis = gpus[i].getAPIs();
            if (apis.supports_cuda) std::cout << "CUDA ";
            if (apis.supports_opencl) std::cout << "OpenCL ";
            if (apis.supports_metal) std::cout << "Metal ";
            if (apis.supports_directx) std::cout << "DirectX ";
            if (apis.supports_vulkan) std::cout << "Vulkan ";
            std::cout << std::endl;
            
            std::cout << "    Memory: " << (gpus[i].getMemory().total_memory_bytes / (1024 * 1024 * 1024)) << " GB" << std::endl;
        }
    }
    
    // Check if CUDA is available
    std::cout << "  CUDA available: " << (resource_manager.isCUDAAvailable() ? "Yes" : "No") << std::endl;
    
    // Check if Metal is available
    std::cout << "  Metal available: " << (resource_manager.isMetalAvailable() ? "Yes" : "No") << std::endl;
}

// Test fixture for resource allocation tests
class ResourceAllocationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize resource manager
        ResourceManager& resource_manager = getResourceManager();
        resource_manager.initialize();
        
        // Initialize configuration manager
        ConfigurationManager& config_manager = getConfigurationManager();
        config_manager.initialize(resource_manager);
    }
};

// Test resource manager
TEST_F(ResourceAllocationTest, ResourceManager) {
    ResourceManager& resource_manager = getResourceManager();
    
    // Test execution strategy
    resource_manager.setExecutionStrategy(ExecutionStrategy::CPU_ONLY);
    EXPECT_EQ(resource_manager.getExecutionStrategy(), ExecutionStrategy::CPU_ONLY);
    
    resource_manager.setExecutionStrategy(ExecutionStrategy::AUTO);
    EXPECT_NE(resource_manager.getExecutionStrategy(), ExecutionStrategy::HYBRID); // HYBRID is not auto-selected
    
    // Test allocation mode
    resource_manager.setAllocationMode(AllocationMode::PERFORMANCE);
    EXPECT_EQ(resource_manager.getAllocationMode(), AllocationMode::PERFORMANCE);
    
    resource_manager.setAllocationMode(AllocationMode::MEMORY);
    EXPECT_EQ(resource_manager.getAllocationMode(), AllocationMode::MEMORY);
    
    // Test optimal thread count
    int thread_count = resource_manager.getOptimalThreadCount();
    EXPECT_GT(thread_count, 0);
    EXPECT_LE(thread_count, resource_manager.getCPUInfo().logical_cores);
    
    // Test optimal block size
    size_t block_size = resource_manager.getOptimalBlockSize();
    EXPECT_GT(block_size, 0);
    
    // Test optimal memory limit
    size_t memory_limit = resource_manager.getOptimalMemoryLimit();
    EXPECT_GT(memory_limit, 0);
    EXPECT_LT(memory_limit, resource_manager.getMemoryInfo().total_physical_memory);
    
    // Print resource manager info
    std::cout << "Resource Manager Info:" << std::endl;
    std::cout << "  Optimal thread count: " << thread_count << std::endl;
    std::cout << "  Optimal block size: " << block_size << " bytes" << std::endl;
    std::cout << "  Optimal memory limit: " << (memory_limit / (1024 * 1024)) << " MB" << std::endl;
    
    // Print system summary
    std::cout << resource_manager.getSystemSummary() << std::endl;
}

// Test configuration manager
TEST_F(ResourceAllocationTest, ConfigurationManager) {
    ConfigurationManager& config_manager = getConfigurationManager();
    
    // Test current profile
    const ConfigProfile& current_profile = config_manager.getCurrentProfile();
    EXPECT_FALSE(current_profile.getName().empty());
    
    // Test available profiles
    const std::map<std::string, ConfigProfile>& profiles = config_manager.getProfiles();
    EXPECT_FALSE(profiles.empty());
    
    // Print configuration summary
    std::cout << config_manager.getConfigurationSummary() << std::endl;
}

// Test MFP creation with different strategies
TEST_F(ResourceAllocationTest, MFPCreation) {
    ResourceManager& resource_manager = getResourceManager();
    
    // Test CPU-only strategy
    resource_manager.setExecutionStrategy(ExecutionStrategy::CPU_ONLY);
    auto mfp_cpu = resource_manager.createMFP(1);
    EXPECT_NE(mfp_cpu, nullptr);
    
    // Test CUDA strategy if available
    if (resource_manager.isCUDAAvailable()) {
        resource_manager.setExecutionStrategy(ExecutionStrategy::CUDA_GPU);
        auto mfp_cuda = resource_manager.createMFP(1);
        EXPECT_NE(mfp_cuda, nullptr);
    }
    
    // Test Metal strategy if available
    if (resource_manager.isMetalAvailable()) {
        resource_manager.setExecutionStrategy(ExecutionStrategy::METAL_GPU);
        auto mfp_metal = resource_manager.createMFP(1);
        EXPECT_NE(mfp_metal, nullptr);
    }
    
    // Test AUTO strategy
    resource_manager.setExecutionStrategy(ExecutionStrategy::AUTO);
    auto mfp_auto = resource_manager.createMFP(1);
    EXPECT_NE(mfp_auto, nullptr);
}

// Test fixture for MFP operations with hardware detection
class MFPOperationsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize resource manager
        ResourceManager& resource_manager = getResourceManager();
        resource_manager.initialize();
        
        // Initialize configuration manager
        ConfigurationManager& config_manager = getConfigurationManager();
        config_manager.initialize(resource_manager);
        
        // Create MFP implementation
        m_mfp = resource_manager.createMFP(1);
    }
    
    std::unique_ptr<MFPBase> m_mfp;
};

// Test isPrime operation
TEST_F(MFPOperationsTest, IsPrime) {
    ASSERT_NE(m_mfp, nullptr);
    
    // Test known primes
    bool is_prime;
    
    m_mfp->isPrime(2, is_prime);
    EXPECT_TRUE(is_prime);
    
    m_mfp->isPrime(3, is_prime);
    EXPECT_TRUE(is_prime);
    
    m_mfp->isPrime(5, is_prime);
    EXPECT_TRUE(is_prime);
    
    m_mfp->isPrime(7, is_prime);
    EXPECT_TRUE(is_prime);
    
    m_mfp->isPrime(11, is_prime);
    EXPECT_TRUE(is_prime);
    
    m_mfp->isPrime(13, is_prime);
    EXPECT_TRUE(is_prime);
    
    // Test known non-primes
    m_mfp->isPrime(1, is_prime);
    EXPECT_FALSE(is_prime);
    
    m_mfp->isPrime(4, is_prime);
    EXPECT_FALSE(is_prime);
    
    m_mfp->isPrime(6, is_prime);
    EXPECT_FALSE(is_prime);
    
    m_mfp->isPrime(8, is_prime);
    EXPECT_FALSE(is_prime);
    
    m_mfp->isPrime(9, is_prime);
    EXPECT_FALSE(is_prime);
    
    m_mfp->isPrime(10, is_prime);
    EXPECT_FALSE(is_prime);
    
    // Test larger prime
    m_mfp->isPrime(104729, is_prime); // 10,000th prime
    EXPECT_TRUE(is_prime);
}

// Test factorize operation
TEST_F(MFPOperationsTest, Factorize) {
    ASSERT_NE(m_mfp, nullptr);
    
    // Test factorization of composite numbers
    std::vector<mpz_t> factors;
    
    // Factorize 4 = 2 * 2
    m_mfp->factorize(4, factors);
    ASSERT_EQ(factors.size(), 2);
    EXPECT_EQ(mpz_get_ui(factors[0]), 2);
    EXPECT_EQ(mpz_get_ui(factors[1]), 2);
    
    // Clear factors
    for (auto& factor : factors) {
        mpz_clear(factor);
    }
    factors.clear();
    
    // Factorize 6 = 2 * 3
    m_mfp->factorize(6, factors);
    ASSERT_EQ(factors.size(), 2);
    EXPECT_EQ(mpz_get_ui(factors[0]), 2);
    EXPECT_EQ(mpz_get_ui(factors[1]), 3);
    
    // Clear factors
    for (auto& factor : factors) {
        mpz_clear(factor);
    }
    factors.clear();
    
    // Factorize 12 = 2 * 2 * 3
    m_mfp->factorize(12, factors);
    ASSERT_EQ(factors.size(), 3);
    EXPECT_EQ(mpz_get_ui(factors[0]), 2);
    EXPECT_EQ(mpz_get_ui(factors[1]), 2);
    EXPECT_EQ(mpz_get_ui(factors[2]), 3);
    
    // Clear factors
    for (auto& factor : factors) {
        mpz_clear(factor);
    }
    factors.clear();
}

// Test nextPrime operation
TEST_F(MFPOperationsTest, NextPrime) {
    ASSERT_NE(m_mfp, nullptr);
    
    // Test next prime
    mpz_t next_prime;
    mpz_init(next_prime);
    
    // Next prime after 1 is 2
    m_mfp->nextPrime(1, next_prime);
    EXPECT_EQ(mpz_get_ui(next_prime), 2);
    
    // Next prime after 2 is 3
    m_mfp->nextPrime(2, next_prime);
    EXPECT_EQ(mpz_get_ui(next_prime), 3);
    
    // Next prime after 3 is 5
    m_mfp->nextPrime(3, next_prime);
    EXPECT_EQ(mpz_get_ui(next_prime), 5);
    
    // Next prime after 5 is 7
    m_mfp->nextPrime(5, next_prime);
    EXPECT_EQ(mpz_get_ui(next_prime), 7);
    
    // Next prime after 7 is 11
    m_mfp->nextPrime(7, next_prime);
    EXPECT_EQ(mpz_get_ui(next_prime), 11);
    
    // Next prime after 11 is 13
    m_mfp->nextPrime(11, next_prime);
    EXPECT_EQ(mpz_get_ui(next_prime), 13);
    
    // Clean up
    mpz_clear(next_prime);
}

// Test performance metrics
TEST_F(MFPOperationsTest, PerformanceMetrics) {
    ASSERT_NE(m_mfp, nullptr);
    
    // Run operations with performance metrics enabled
    m_mfp->enablePerformanceMetrics(true);
    
    // Run isPrime operation
    bool is_prime;
    m_mfp->isPrime(104729, is_prime); // 10,000th prime
    
    // Get performance metrics
    const PerformanceMetrics& metrics = m_mfp->getPerformanceMetrics();
    
    // Basic validation
    EXPECT_GT(metrics.total_execution_time_ms, 0.0);
    EXPECT_GT(metrics.operations_count, 0);
    
    // Print performance metrics
    std::cout << "Performance Metrics:" << std::endl;
    std::cout << "  Total execution time: " << metrics.total_execution_time_ms << " ms" << std::endl;
    std::cout << "  Operations count: " << metrics.operations_count << std::endl;
    std::cout << "  Average time per operation: " << (metrics.total_execution_time_ms / metrics.operations_count) << " ms" << std::endl;
}

// Test all MFP methods with hardware detection
TEST_F(MFPOperationsTest, AllMethods) {
    ResourceManager& resource_manager = getResourceManager();
    
    // Test all three MFP methods
    for (int method = 1; method <= 3; method++) {
        std::cout << "Testing MFP Method " << method << std::endl;
        
        auto mfp = resource_manager.createMFP(method);
        ASSERT_NE(mfp, nullptr);
        
        // Enable performance metrics
        mfp->enablePerformanceMetrics(true);
        
        // Test isPrime operation
        bool is_prime;
        mfp->isPrime(104729, is_prime); // 10,000th prime
        EXPECT_TRUE(is_prime);
        
        // Get performance metrics
        const PerformanceMetrics& metrics = mfp->getPerformanceMetrics();
        
        // Print performance metrics
        std::cout << "  Method " << method << " execution time: " << metrics.total_execution_time_ms << " ms" << std::endl;
    }
}

} // namespace test
} // namespace mfp

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
