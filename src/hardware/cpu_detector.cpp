#include "hardware/cpu_detector.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <thread>
#include <iostream>

#ifdef __linux__
#include <unistd.h>
#include <sys/sysinfo.h>
#include <sys/stat.h>
#include <fcntl.h>
#elif defined(__APPLE__)
#include <sys/sysctl.h>
#include <mach/mach.h>
#include <mach/machine.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

namespace mfp {

CPUInfo::CPUInfo() {
    // Initialize with defaults
}

CPUInfo::~CPUInfo() {
    // Nothing to clean up
}

void CPUInfo::detect() {
#ifdef __linux__
    detectOnLinux();
#elif defined(__APPLE__)
    detectOnMacOS();
#elif defined(_WIN32)
    detectOnWindows();
#else
    // Fallback to basic detection
    m_topology.logical_cores = std::thread::hardware_concurrency();
    m_topology.physical_cores = m_topology.logical_cores;
#endif
}

int CPUInfo::getOptimalThreadCount(bool memory_intensive, bool io_intensive) const {
    // Start with logical core count
    int thread_count = m_topology.logical_cores;
    
    // For memory-intensive workloads, use physical cores to avoid cache contention
    if (memory_intensive && m_topology.has_hyperthreading) {
        thread_count = m_topology.physical_cores;
    }
    
    // For I/O-intensive workloads, use more threads to hide latency
    if (io_intensive) {
        thread_count = m_topology.logical_cores * 2;
    }
    
    // Ensure at least one thread
    if (thread_count < 1) {
        thread_count = 1;
    }
    
    return thread_count;
}

bool CPUInfo::hasFeature(const std::string& feature_name) const {
    if (feature_name == "sse") return m_features.has_sse;
    if (feature_name == "sse2") return m_features.has_sse2;
    if (feature_name == "sse3") return m_features.has_sse3;
    if (feature_name == "ssse3") return m_features.has_ssse3;
    if (feature_name == "sse4.1") return m_features.has_sse4_1;
    if (feature_name == "sse4.2") return m_features.has_sse4_2;
    if (feature_name == "avx") return m_features.has_avx;
    if (feature_name == "avx2") return m_features.has_avx2;
    if (feature_name == "avx512f") return m_features.has_avx512f;
    if (feature_name == "neon") return m_features.has_neon;
    if (feature_name == "sve") return m_features.has_sve;
    
    return false;
}

std::string CPUInfo::getSummary() const {
    std::stringstream ss;
    
    ss << "CPU Information:" << std::endl;
    ss << "  Vendor: " << m_vendor << std::endl;
    ss << "  Model: " << m_model << std::endl;
    
    ss << "  Architecture: ";
    switch (m_architecture) {
        case CPUArchitecture::X86: ss << "x86"; break;
        case CPUArchitecture::X86_64: ss << "x86_64"; break;
        case CPUArchitecture::ARM: ss << "ARM"; break;
        case CPUArchitecture::ARM64: ss << "ARM64"; break;
        case CPUArchitecture::PPC: ss << "PowerPC"; break;
        case CPUArchitecture::PPC64: ss << "PowerPC 64"; break;
        case CPUArchitecture::MIPS: ss << "MIPS"; break;
        case CPUArchitecture::RISC_V: ss << "RISC-V"; break;
        default: ss << "Unknown"; break;
    }
    ss << std::endl;
    
    ss << "  Cores: " << m_topology.physical_cores << " physical, " 
       << m_topology.logical_cores << " logical" << std::endl;
    
    ss << "  Frequency: " << m_frequency.base_mhz << " MHz base";
    if (m_frequency.max_mhz > 0) {
        ss << ", " << m_frequency.max_mhz << " MHz max";
    }
    ss << std::endl;
    
    ss << "  Cache: ";
    if (m_cache.l1_data_size_kb > 0) {
        ss << "L1 " << m_cache.l1_data_size_kb << "KB, ";
    }
    if (m_cache.l2_size_kb > 0) {
        ss << "L2 " << m_cache.l2_size_kb << "KB, ";
    }
    if (m_cache.l3_size_kb > 0) {
        ss << "L3 " << m_cache.l3_size_kb << "KB";
    }
    ss << std::endl;
    
    ss << "  Features: ";
    if (m_features.has_sse) ss << "SSE ";
    if (m_features.has_sse2) ss << "SSE2 ";
    if (m_features.has_sse3) ss << "SSE3 ";
    if (m_features.has_ssse3) ss << "SSSE3 ";
    if (m_features.has_sse4_1) ss << "SSE4.1 ";
    if (m_features.has_sse4_2) ss << "SSE4.2 ";
    if (m_features.has_avx) ss << "AVX ";
    if (m_features.has_avx2) ss << "AVX2 ";
    if (m_features.has_avx512f) ss << "AVX512F ";
    if (m_features.has_neon) ss << "NEON ";
    if (m_features.has_sve) ss << "SVE ";
    ss << std::endl;
    
    return ss.str();
}

void CPUInfo::detectOnLinux() {
    // Read /proc/cpuinfo
    std::ifstream cpuinfo("/proc/cpuinfo");
    if (!cpuinfo.is_open()) {
        return;
    }
    
    std::string line;
    int processor_count = 0;
    std::string flags;
    
    while (std::getline(cpuinfo, line)) {
        if (line.find("processor") == 0) {
            processor_count++;
        } else if (line.find("vendor_id") == 0) {
            m_vendor = line.substr(line.find(":") + 2);
        } else if (line.find("model name") == 0) {
            m_model = line.substr(line.find(":") + 2);
        } else if (line.find("flags") == 0 || line.find("Features") == 0) {
            flags = line.substr(line.find(":") + 2);
        }
    }
    
    // Set architecture
    #if defined(__x86_64__) || defined(_M_X64)
        m_architecture = CPUArchitecture::X86_64;
    #elif defined(__i386__) || defined(_M_IX86)
        m_architecture = CPUArchitecture::X86;
    #elif defined(__aarch64__) || defined(_M_ARM64)
        m_architecture = CPUArchitecture::ARM64;
    #elif defined(__arm__) || defined(_M_ARM)
        m_architecture = CPUArchitecture::ARM;
    #elif defined(__powerpc64__)
        m_architecture = CPUArchitecture::PPC64;
    #elif defined(__powerpc__)
        m_architecture = CPUArchitecture::PPC;
    #elif defined(__mips__)
        m_architecture = CPUArchitecture::MIPS;
    #elif defined(__riscv)
        m_architecture = CPUArchitecture::RISC_V;
    #else
        m_architecture = CPUArchitecture::UNKNOWN;
    #endif
    
    // Set topology
    m_topology.logical_cores = processor_count;
    
    // Try to get physical core count
    std::ifstream cpu_present("/sys/devices/system/cpu/present");
    if (cpu_present.is_open()) {
        std::string line;
        std::getline(cpu_present, line);
        
        // Parse range like "0-31"
        std::regex range_regex("(\\d+)-(\\d+)");
        std::smatch match;
        if (std::regex_search(line, match, range_regex) && match.size() > 2) {
            int min_cpu = std::stoi(match[1].str());
            int max_cpu = std::stoi(match[2].str());
            m_topology.logical_cores = max_cpu - min_cpu + 1;
        }
    }
    
    // Try to get physical core count from topology
    int physical_cores = 0;
    std::set<std::string> core_ids;
    
    for (int i = 0; i < m_topology.logical_cores; i++) {
        std::stringstream ss;
        ss << "/sys/devices/system/cpu/cpu" << i << "/topology/core_id";
        std::ifstream core_id_file(ss.str());
        if (core_id_file.is_open()) {
            std::string core_id;
            std::getline(core_id_file, core_id);
            core_ids.insert(core_id);
        }
    }
    
    if (!core_ids.empty()) {
        m_topology.physical_cores = core_ids.size();
    } else {
        // Fallback: assume all cores are physical
        m_topology.physical_cores = m_topology.logical_cores;
    }
    
    // Detect hyperthreading
    m_topology.has_hyperthreading = (m_topology.logical_cores > m_topology.physical_cores);
    
    // Parse CPU flags
    m_features.has_sse = flags.find(" sse ") != std::string::npos;
    m_features.has_sse2 = flags.find(" sse2 ") != std::string::npos;
    m_features.has_sse3 = flags.find(" sse3 ") != std::string::npos;
    m_features.has_ssse3 = flags.find(" ssse3 ") != std::string::npos;
    m_features.has_sse4_1 = flags.find(" sse4_1 ") != std::string::npos;
    m_features.has_sse4_2 = flags.find(" sse4_2 ") != std::string::npos;
    m_features.has_avx = flags.find(" avx ") != std::string::npos;
    m_features.has_avx2 = flags.find(" avx2 ") != std::string::npos;
    m_features.has_avx512f = flags.find(" avx512f ") != std::string::npos;
    m_features.has_neon = flags.find(" neon ") != std::string::npos;
    m_features.has_sve = flags.find(" sve ") != std::string::npos;
    
    // Get CPU frequency
    std::ifstream cpu_freq("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
    if (cpu_freq.is_open()) {
        std::string freq_str;
        std::getline(cpu_freq, freq_str);
        m_frequency.max_mhz = std::stod(freq_str) / 1000.0;
    }
    
    std::ifstream cpu_min_freq("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_min_freq");
    if (cpu_min_freq.is_open()) {
        std::string freq_str;
        std::getline(cpu_min_freq, freq_str);
        m_frequency.min_mhz = std::stod(freq_str) / 1000.0;
    }
    
    // Base frequency is harder to get, use max as fallback
    m_frequency.base_mhz = m_frequency.max_mhz;
    
    // Get cache information
    std::ifstream l1d_cache("/sys/devices/system/cpu/cpu0/cache/index0/size");
    if (l1d_cache.is_open()) {
        std::string size_str;
        std::getline(l1d_cache, size_str);
        m_cache.l1_data_size_kb = std::stoi(size_str);
    }
    
    std::ifstream l1i_cache("/sys/devices/system/cpu/cpu0/cache/index1/size");
    if (l1i_cache.is_open()) {
        std::string size_str;
        std::getline(l1i_cache, size_str);
        m_cache.l1_instruction_size_kb = std::stoi(size_str);
    }
    
    std::ifstream l2_cache("/sys/devices/system/cpu/cpu0/cache/index2/size");
    if (l2_cache.is_open()) {
        std::string size_str;
        std::getline(l2_cache, size_str);
        m_cache.l2_size_kb = std::stoi(size_str);
    }
    
    std::ifstream l3_cache("/sys/devices/system/cpu/cpu0/cache/index3/size");
    if (l3_cache.is_open()) {
        std::string size_str;
        std::getline(l3_cache, size_str);
        m_cache.l3_size_kb = std::stoi(size_str);
    }
}

void CPUInfo::detectOnMacOS() {
#ifdef __APPLE__
    // Get CPU brand string
    char brand[256];
    size_t size = sizeof(brand);
    if (sysctlbyname("machdep.cpu.brand_string", &brand, &size, nullptr, 0) == 0) {
        m_model = brand;
    }
    
    // Get vendor
    char vendor[256];
    size = sizeof(vendor);
    if (sysctlbyname("machdep.cpu.vendor", &vendor, &size, nullptr, 0) == 0) {
        m_vendor = vendor;
    }
    
    // Get logical core count
    int logical_cores;
    size = sizeof(logical_cores);
    if (sysctlbyname("hw.logicalcpu", &logical_cores, &size, nullptr, 0) == 0) {
        m_topology.logical_cores = logical_cores;
    } else {
        m_topology.logical_cores = std::thread::hardware_concurrency();
    }
    
    // Get physical core count
    int physical_cores;
    size = sizeof(physical_cores);
    if (sysctlbyname("hw.physicalcpu", &physical_cores, &size, nullptr, 0) == 0) {
        m_topology.physical_cores = physical_cores;
    } else {
        m_topology.physical_cores = m_topology.logical_cores;
    }
    
    // Detect hyperthreading
    m_topology.has_hyperthreading = (m_topology.logical_cores > m_topology.physical_cores);
    
    // Get CPU frequency
    int64_t freq;
    size = sizeof(freq);
    if (sysctlbyname("hw.cpufrequency", &freq, &size, nullptr, 0) == 0) {
        m_frequency.base_mhz = static_cast<double>(freq) / 1000000.0;
    }
    
    size = sizeof(freq);
    if (sysctlbyname("hw.cpufrequency_max", &freq, &size, nullptr, 0) == 0) {
        m_frequency.max_mhz = static_cast<double>(freq) / 1000000.0;
    }
    
    size = sizeof(freq);
    if (sysctlbyname("hw.cpufrequency_min", &freq, &size, nullptr, 0) == 0) {
        m_frequency.min_mhz = static_cast<double>(freq) / 1000000.0;
    }
    
    // Get cache information
    int64_t cache_size;
    size = sizeof(cache_size);
    if (sysctlbyname("hw.l1dcachesize", &cache_size, &size, nullptr, 0) == 0) {
        m_cache.l1_data_size_kb = static_cast<int>(cache_size / 1024);
    }
    
    size = sizeof(cache_size);
    if (sysctlbyname("hw.l1icachesize", &cache_size, &size, nullptr, 0) == 0) {
        m_cache.l1_instruction_size_kb = static_cast<int>(cache_size / 1024);
    }
    
    size = sizeof(cache_size);
    if (sysctlbyname("hw.l2cachesize", &cache_size, &size, nullptr, 0) == 0) {
        m_cache.l2_size_kb = static_cast<int>(cache_size / 1024);
    }
    
    size = sizeof(cache_size);
    if (sysctlbyname("hw.l3cachesize", &cache_size, &size, nullptr, 0) == 0) {
        m_cache.l3_size_kb = static_cast<int>(cache_size / 1024);
    }
    
    // Detect architecture
    cpu_type_t cpu_type;
    cpu_subtype_t cpu_subtype;
    size = sizeof(cpu_type);
    sysctlbyname("hw.cputype", &cpu_type, &size, nullptr, 0);
    size = sizeof(cpu_subtype);
    sysctlbyname("hw.cpusubtype", &cpu_subtype, &size, nullptr, 0);
    
    if (cpu_type == CPU_TYPE_X86_64) {
        m_architecture = CPUArchitecture::X86_64;
    } else if (cpu_type == CPU_TYPE_X86) {
        m_architecture = CPUArchitecture::X86;
    } else if (cpu_type == CPU_TYPE_ARM64) {
        m_architecture = CPUArchitecture::ARM64;
    } else if (cpu_type == CPU_TYPE_ARM) {
        m_architecture = CPUArchitecture::ARM;
    } else {
        m_architecture = CPUArchitecture::UNKNOWN;
    }
    
    // Detect CPU features
    int has_feature;
    size = sizeof(has_feature);
    
    if (sysctlbyname("hw.optional.sse", &has_feature, &size, nullptr, 0) == 0) {
        m_features.has_sse = (has_feature == 1);
    }
    
    if (sysctlbyname("hw.optional.sse2", &has_feature, &size, nullptr, 0) == 0) {
        m_features.has_sse2 = (has_feature == 1);
    }
    
    if (sysctlbyname("hw.optional.sse3", &has_feature, &size, nullptr, 0) == 0) {
        m_features.has_sse3 = (has_feature == 1);
    }
    
    if (sysctlbyname("hw.optional.supplementalsse3", &has_feature, &size, nullptr, 0) == 0) {
        m_features.has_ssse3 = (has_feature == 1);
    }
    
    if (sysctlbyname("hw.optional.sse4_1", &has_feature, &size, nullptr, 0) == 0) {
        m_features.has_sse4_1 = (has_feature == 1);
    }
    
    if (sysctlbyname("hw.optional.sse4_2", &has_feature, &size, nullptr, 0) == 0) {
        m_features.has_sse4_2 = (has_feature == 1);
    }
    
    if (sysctlbyname("hw.optional.avx1_0", &has_feature, &size, nullptr, 0) == 0) {
        m_features.has_avx = (has_feature == 1);
    }
    
    if (sysctlbyname("hw.optional.avx2_0", &has_feature, &size, nullptr, 0) == 0) {
        m_features.has_avx2 = (has_feature == 1);
    }
    
    // For ARM Macs
    if (m_architecture == CPUArchitecture::ARM64) {
        m_features.has_neon = true;  // All ARM64 CPUs have NEON
    }
#endif
}

void CPUInfo::detectOnWindows() {
#ifdef _WIN32
    // Get logical processor count
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    m_topology.logical_cores = sysInfo.dwNumberOfProcessors;
    
    // Get processor architecture
    switch (sysInfo.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_AMD64:
            m_architecture = CPUArchitecture::X86_64;
            break;
        case PROCESSOR_ARCHITECTURE_INTEL:
            m_architecture = CPUArchitecture::X86;
            break;
        case PROCESSOR_ARCHITECTURE_ARM:
            m_architecture = CPUArchitecture::ARM;
            break;
        case PROCESSOR_ARCHITECTURE_ARM64:
            m_architecture = CPUArchitecture::ARM64;
            break;
        default:
            m_architecture = CPUArchitecture::UNKNOWN;
            break;
    }
    
    // Get processor information using CPUID
    int cpuInfo[4] = {-1};
    char vendorID[13] = {0};
    
    // Get vendor ID
    __cpuid(cpuInfo, 0);
    memcpy(vendorID, &cpuInfo[1], 4);
    memcpy(vendorID + 4, &cpuInfo[3], 4);
    memcpy(vendorID + 8, &cpuInfo[2], 4);
    m_vendor = vendorID;
    
    // Get processor brand string
    char brand[49] = {0};
    __cpuid(cpuInfo, 0x80000000);
    unsigned int nExIds = cpuInfo[0];
    
    if (nExIds >= 0x80000004) {
        for (unsigned int i = 0; i < 3; ++i) {
            __cpuid(cpuInfo, 0x80000002 + i);
            memcpy(brand + i * 16, cpuInfo, 16);
        }
        m_model = brand;
    }
    
    // Get feature flags
    __cpuid(cpuInfo, 1);
    m_features.has_sse = (cpuInfo[3] & (1 << 25)) != 0;
    m_features.has_sse2 = (cpuInfo[3] & (1 << 26)) != 0;
    m_features.has_sse3 = (cpuInfo[2] & (1 << 0)) != 0;
    m_features.has_ssse3 = (cpuInfo[2] & (1 << 9)) != 0;
    m_features.has_sse4_1 = (cpuInfo[2] & (1 << 19)) != 0;
    m_features.has_sse4_2 = (cpuInfo[2] & (1 << 20)) != 0;
    m_features.has_avx = (cpuInfo[2] & (1 << 28)) != 0;
    
    // Check for AVX2
    __cpuid(cpuInfo, 7);
    m_features.has_avx2 = (cpuInfo[1] & (1 << 5)) != 0;
    m_features.has_avx512f = (cpuInfo[1] & (1 << 16)) != 0;
    
    // Get physical core count
    DWORD length = 0;
    DWORD buffer[4] = {0};
    
    GetLogicalProcessorInformation(nullptr, &length);
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> buffer(length / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
        if (GetLogicalProcessorInformation(&buffer[0], &length)) {
            int physical_cores = 0;
            for (size_t i = 0; i < buffer.size(); ++i) {
                if (buffer[i].Relationship == RelationProcessorCore) {
                    physical_cores++;
                }
            }
            m_topology.physical_cores = physical_cores;
        }
    }
    
    // Detect hyperthreading
    m_topology.has_hyperthreading = (m_topology.logical_cores > m_topology.physical_cores);
    
    // Get CPU frequency
    HKEY hKey;
    DWORD dwMHz = 0;
    DWORD dwSize = sizeof(dwMHz);
    
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueEx(hKey, "~MHz", NULL, NULL, (LPBYTE)&dwMHz, &dwSize) == ERROR_SUCCESS) {
            m_frequency.base_mhz = static_cast<double>(dwMHz);
        }
        RegCloseKey(hKey);
    }
#endif
}

} // namespace mfp
