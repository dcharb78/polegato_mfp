#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace mfp {
namespace system {

// CPU architecture enumeration
enum class CPUArchitecture {
    X86,
    X86_64,
    ARM,
    ARM64,
    PPC,
    PPC64,
    UNKNOWN
};

// CPU feature enumeration
enum class CPUFeature {
    SSE, SSE2, SSE3, SSSE3, SSE41, SSE42,
    AVX, AVX2, AVX512F, AVX512BW, AVX512CD, AVX512DQ, AVX512VL,
    FMA, BMI1, BMI2, AES, SHA,
    RDRAND, RDSEED, ADX, PREFETCHW,
    F16C, POPCNT, LZCNT
};

// Workload type enumeration
enum class WorkloadType {
    COMPUTE_INTENSIVE,
    MEMORY_INTENSIVE,
    IO_INTENSIVE,
    BALANCED
};

// Cache information structure
struct CacheInfo {
    int l1d_size_kb = 0;
    int l1i_size_kb = 0;
    int l2_size_kb = 0;
    int l3_size_kb = 0;
    int l1d_line_size = 0;
    int l1i_line_size = 0;
    int l2_line_size = 0;
    int l3_line_size = 0;
    int l1d_associativity = 0;
    int l1i_associativity = 0;
    int l2_associativity = 0;
    int l3_associativity = 0;
};

// Frequency information structure
struct FrequencyInfo {
    double base_frequency_mhz = 0.0;
    double max_frequency_mhz = 0.0;
    double min_frequency_mhz = 0.0;
    bool turbo_boost = false;
    int max_turbo_frequency_mhz = 0;
};

// CPU topology information
struct TopologyInfo {
    int physical_cores = 0;
    int logical_cores = 0;
    int sockets = 0;
    int cores_per_socket = 0;
    int numa_nodes = 0;
    bool hyper_threading = false;
    std::vector<std::vector<int>> numa_cpu_mapping;
};

// CPU information structure
struct CPUInfo {
    CPUArchitecture architecture = CPUArchitecture::UNKNOWN;
    std::string vendor;
    std::string model_name;
    std::string family;
    std::string model;
    std::string stepping;
    TopologyInfo topology;
    std::vector<CPUFeature> features;
    CacheInfo cache_info;
    FrequencyInfo frequency_info;
    std::map<std::string, std::string> additional_info;
};

class CPUDetector {
public:
    // Constructor and destructor
    CPUDetector();
    ~CPUDetector();
    
    // Detect CPU capabilities
    bool detect();
    
    // Get CPU information
    const CPUInfo& getCPUInfo() const;
    
    // Check for specific CPU features
    bool hasFeature(CPUFeature feature) const;
    
    // Get optimal thread count for different workloads
    int getOptimalThreadCount(WorkloadType workload) const;
    
    // Get CPU architecture
    CPUArchitecture getArchitecture() const;
    
    // Get cache information
    const CacheInfo& getCacheInfo() const;
    
    // Get CPU frequency information
    const FrequencyInfo& getFrequencyInfo() const;
    
    // Get CPU topology information
    const TopologyInfo& getTopologyInfo() const;
    
    // Get CPU feature string
    std::string getFeatureString() const;
    
    // Get CPU summary
    std::string getSummary() const;
    
private:
    // CPU information
    CPUInfo cpu_info_;
    
    // Detection methods
    bool detectArchitecture();
    bool detectVendorAndModel();
    bool detectTopology();
    bool detectCPUFeatures();
    bool detectCacheInfo();
    bool detectFrequencyInfo();
    
    // Platform-specific detection
    bool detectCPUInfoLinux();
    bool detectCPUInfoWindows();
    bool detectCPUInfoMacOS();
    
    // Helper methods
    bool parseCPUFlags(const std::string& flags);
    bool readCPUInfoFile();
    bool executeCommand(const std::string& command, std::string& output);
    std::string featureToString(CPUFeature feature) const;
    CPUFeature stringToFeature(const std::string& feature_str) const;
    
    // Feature detection helpers
    bool detectSSE();
    bool detectAVX();
    bool detectAVX2();
    bool detectAVX512();
    
    // Thread count optimization
    int calculateOptimalThreadCount(WorkloadType workload) const;
    
    // State
    bool initialized_ = false;
};

} // namespace system
} // namespace mfp
