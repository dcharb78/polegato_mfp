#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace mfp {
namespace system {

// Forward declarations
class CPUDetector;
class MemoryDetector;
class StorageDetector;

// GPU vendor enumeration
enum class GPUVendor {
    NVIDIA,
    AMD,
    INTEL,
    APPLE,
    UNKNOWN
};

// GPU architecture enumeration
enum class GPUArchitecture {
    // NVIDIA architectures
    KEPLER,
    MAXWELL,
    PASCAL,
    VOLTA,
    TURING,
    AMPERE,
    ADA_LOVELACE,
    HOPPER,
    
    // AMD architectures
    GCN,
    RDNA,
    RDNA2,
    RDNA3,
    
    // Intel architectures
    GEN9,
    GEN11,
    XE,
    ARC,
    
    // Apple architectures
    M1,
    M2,
    M3,
    
    UNKNOWN
};

// GPU API support enumeration
enum class GPUAPISupport {
    CUDA,
    OPENCL,
    METAL,
    DIRECTX,
    VULKAN,
    NONE
};

// GPU memory information structure
struct GPUMemoryInfo {
    size_t total_memory_bytes = 0;
    size_t free_memory_bytes = 0;
    size_t used_memory_bytes = 0;
    double memory_clock_mhz = 0.0;
    double memory_bandwidth_gbps = 0.0;
    bool ecc_enabled = false;
    std::string memory_type;
    int memory_bus_width = 0;
};

// GPU compute capability structure
struct GPUComputeInfo {
    int cuda_cores = 0;
    int tensor_cores = 0;
    int rt_cores = 0;
    int compute_units = 0;
    int stream_processors = 0;
    double core_clock_mhz = 0.0;
    double boost_clock_mhz = 0.0;
    double theoretical_tflops_fp32 = 0.0;
    double theoretical_tflops_fp16 = 0.0;
    double theoretical_tflops_int8 = 0.0;
    std::string cuda_compute_capability;
    int opencl_version_major = 0;
    int opencl_version_minor = 0;
    bool supports_unified_memory = false;
};

// GPU information structure
struct GPUInfo {
    int device_id = 0;
    std::string name;
    GPUVendor vendor = GPUVendor::UNKNOWN;
    GPUArchitecture architecture = GPUArchitecture::UNKNOWN;
    std::string driver_version;
    std::vector<GPUAPISupport> api_support;
    GPUMemoryInfo memory_info;
    GPUComputeInfo compute_info;
    int pcie_generation = 0;
    int pcie_lanes = 0;
    double power_usage_watts = 0.0;
    double max_power_watts = 0.0;
    double temperature_celsius = 0.0;
    bool is_integrated = false;
    std::map<std::string, std::string> additional_info;
};

// GPU detector class
class GPUDetector {
public:
    // Constructor and destructor
    GPUDetector();
    ~GPUDetector();
    
    // Detect GPU capabilities
    bool detect();
    
    // Get GPU information
    const std::vector<GPUInfo>& getGPUInfo() const;
    
    // Get number of GPUs
    int getGPUCount() const;
    
    // Get GPU by index
    const GPUInfo* getGPUByIndex(int index) const;
    
    // Get best GPU for compute
    const GPUInfo* getBestGPUForCompute() const;
    
    // Check if CUDA is available
    bool hasCUDASupport() const;
    
    // Check if Metal is available
    bool hasMetalSupport() const;
    
    // Get GPU summary
    std::string getSummary() const;
    
    // Set CPU and Memory detectors for context
    void setCPUDetector(const CPUDetector* cpu_detector);
    void setMemoryDetector(const MemoryDetector* memory_detector);
    
private:
    // GPU information
    std::vector<GPUInfo> gpu_info_;
    
    // Detection methods
    bool detectNVIDIAGPUs();
    bool detectAMDGPUs();
    bool detectIntelGPUs();
    bool detectAppleGPUs();
    
    // Platform-specific detection
    bool detectGPUsLinux();
    bool detectGPUsWindows();
    bool detectGPUsMacOS();
    
    // Helper methods
    bool detectCUDAGPUs();
    bool detectOpenCLGPUs();
    bool detectMetalGPUs();
    bool detectVulkanGPUs();
    bool detectDirectXGPUs();
    
    bool executeCommand(const std::string& command, std::string& output);
    GPUArchitecture determineNVIDIAArchitecture(const std::string& device_name, const std::string& compute_capability);
    GPUArchitecture determineAMDArchitecture(const std::string& device_name);
    GPUArchitecture determineIntelArchitecture(const std::string& device_name);
    GPUArchitecture determineAppleArchitecture(const std::string& device_name);
    
    // Context from other detectors
    const CPUDetector* cpu_detector_ = nullptr;
    const MemoryDetector* memory_detector_ = nullptr;
    
    // State
    bool initialized_ = false;
};

} // namespace system
} // namespace mfp
