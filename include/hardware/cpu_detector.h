#pragma once

#include <string>
#include <vector>
#include <thread>

namespace mfp {

// CPU architecture types
enum class CPUArchitecture {
    UNKNOWN,
    X86,
    X86_64,
    ARM,
    ARM64,
    PPC,
    PPC64,
    MIPS,
    RISC_V
};

// CPU feature flags
struct CPUFeatures {
    bool has_sse = false;
    bool has_sse2 = false;
    bool has_sse3 = false;
    bool has_ssse3 = false;
    bool has_sse4_1 = false;
    bool has_sse4_2 = false;
    bool has_avx = false;
    bool has_avx2 = false;
    bool has_avx512f = false;
    bool has_neon = false;  // ARM
    bool has_sve = false;   // ARM
};

// CPU topology information
struct CPUTopology {
    int physical_cores = 0;
    int logical_cores = 0;
    int numa_nodes = 0;
    bool has_hyperthreading = false;
};

// CPU cache information
struct CPUCache {
    int l1_data_size_kb = 0;
    int l1_instruction_size_kb = 0;
    int l2_size_kb = 0;
    int l3_size_kb = 0;
    int l4_size_kb = 0;
    int line_size = 0;
};

// CPU frequency information
struct CPUFrequency {
    double base_mhz = 0.0;
    double max_mhz = 0.0;
    double min_mhz = 0.0;
};

// Complete CPU information
class CPUInfo {
public:
    CPUInfo();
    ~CPUInfo();

    // Detect CPU capabilities
    void detect();

    // Getters
    CPUArchitecture getArchitecture() const { return m_architecture; }
    const std::string& getVendor() const { return m_vendor; }
    const std::string& getModel() const { return m_model; }
    const CPUFeatures& getFeatures() const { return m_features; }
    const CPUTopology& getTopology() const { return m_topology; }
    const CPUCache& getCache() const { return m_cache; }
    const CPUFrequency& getFrequency() const { return m_frequency; }

    // Calculate optimal thread count for a specific workload
    int getOptimalThreadCount(bool memory_intensive = false, bool io_intensive = false) const;

    // Check if specific feature is available
    bool hasFeature(const std::string& feature_name) const;

    // Get a human-readable summary
    std::string getSummary() const;

private:
    CPUArchitecture m_architecture = CPUArchitecture::UNKNOWN;
    std::string m_vendor;
    std::string m_model;
    CPUFeatures m_features;
    CPUTopology m_topology;
    CPUCache m_cache;
    CPUFrequency m_frequency;

    // Platform-specific detection methods
    void detectOnLinux();
    void detectOnMacOS();
    void detectOnWindows();
};

} // namespace mfp
