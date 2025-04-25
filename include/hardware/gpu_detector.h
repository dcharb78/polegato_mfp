#pragma once

#include <string>
#include <vector>
#include <memory>

namespace mfp {

// GPU vendor types
enum class GPUVendor {
    UNKNOWN,
    NVIDIA,
    AMD,
    INTEL,
    APPLE
};

// GPU API support
struct GPUAPIs {
    bool supports_cuda = false;
    bool supports_opencl = false;
    bool supports_metal = false;
    bool supports_directx = false;
    bool supports_vulkan = false;
};

// GPU memory information
struct GPUMemory {
    uint64_t total_memory_bytes = 0;
    uint64_t available_memory_bytes = 0;
    double memory_clock_mhz = 0.0;
    double memory_bandwidth_gbps = 0.0;
};

// GPU compute capabilities
struct GPUCompute {
    int compute_units = 0;
    int cuda_cores = 0;
    int tensor_cores = 0;
    int rt_cores = 0;
    double clock_mhz = 0.0;
    double tflops_fp32 = 0.0;
    double tflops_fp16 = 0.0;
    int cuda_compute_capability_major = 0;
    int cuda_compute_capability_minor = 0;
};

// Complete GPU information
class GPUInfo {
public:
    GPUInfo();
    ~GPUInfo();
    
    // Getters
    GPUVendor getVendor() const { return m_vendor; }
    const std::string& getName() const { return m_name; }
    const std::string& getDriverVersion() const { return m_driver_version; }
    const GPUAPIs& getAPIs() const { return m_apis; }
    const GPUMemory& getMemory() const { return m_memory; }
    const GPUCompute& getCompute() const { return m_compute; }
    bool isIntegrated() const { return m_is_integrated; }
    
    // Setters (for internal use)
    void setVendor(GPUVendor vendor) { m_vendor = vendor; }
    void setName(const std::string& name) { m_name = name; }
    void setDriverVersion(const std::string& version) { m_driver_version = version; }
    void setAPIs(const GPUAPIs& apis) { m_apis = apis; }
    void setMemory(const GPUMemory& memory) { m_memory = memory; }
    void setCompute(const GPUCompute& compute) { m_compute = compute; }
    void setIntegrated(bool integrated) { m_is_integrated = integrated; }
    
    // Get a human-readable summary
    std::string getSummary() const;
    
private:
    GPUVendor m_vendor = GPUVendor::UNKNOWN;
    std::string m_name;
    std::string m_driver_version;
    GPUAPIs m_apis;
    GPUMemory m_memory;
    GPUCompute m_compute;
    bool m_is_integrated = false;
};

// GPU detector class
class GPUDetector {
public:
    GPUDetector();
    ~GPUDetector();
    
    // Detect all available GPUs
    void detect();
    
    // Getters
    const std::vector<GPUInfo>& getGPUs() const { return m_gpus; }
    bool hasGPU() const { return !m_gpus.empty(); }
    bool hasCUDAGPU() const;
    bool hasMetalGPU() const;
    
    // Find the best GPU for compute operations
    GPUInfo findBestComputeGPU() const;
    
    // Get a human-readable summary of all GPUs
    std::string getSummary() const;
    
private:
    std::vector<GPUInfo> m_gpus;
    
    // Platform-specific detection methods
    void detectCUDAGPUs();
    void detectOpenCLGPUs();
    void detectMetalGPUs();
    void detectDirectXGPUs();
    void detectVulkanGPUs();
    
    // Fallback detection using command-line tools
    void detectUsingCommandLine();
    
    // Helper methods
    void mergeGPUInfo();
    void calculatePerformanceMetrics();
};

} // namespace mfp
