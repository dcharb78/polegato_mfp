#include "cpu_detector.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <thread>
#include <cstring>
#include <cstdlib>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#include <intrin.h>
#elif defined(__APPLE__)
#include <sys/sysctl.h>
#include <mach/mach.h>
#include <mach/processor_info.h>
#include <mach/mach_host.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

namespace mfp {
namespace system {

CPUDetector::CPUDetector() {
    // Initialize with default values
}

CPUDetector::~CPUDetector() {
    // Clean up resources if needed
}

bool CPUDetector::detect() {
    if (initialized_) {
        return true; // Already initialized
    }
    
    bool success = true;
    
    // Detect basic CPU information
    success &= detectArchitecture();
    success &= detectVendorAndModel();
    success &= detectTopology();
    success &= detectCPUFeatures();
    success &= detectCacheInfo();
    success &= detectFrequencyInfo();
    
    initialized_ = success;
    return success;
}

const CPUInfo& CPUDetector::getCPUInfo() const {
    return cpu_info_;
}

bool CPUDetector::hasFeature(CPUFeature feature) const {
    if (!initialized_) {
        return false;
    }
    
    return std::find(cpu_info_.features.begin(), cpu_info_.features.end(), feature) != cpu_info_.features.end();
}

int CPUDetector::getOptimalThreadCount(WorkloadType workload) const {
    if (!initialized_) {
        // Default to hardware concurrency if not initialized
        return std::thread::hardware_concurrency();
    }
    
    return calculateOptimalThreadCount(workload);
}

CPUArchitecture CPUDetector::getArchitecture() const {
    return cpu_info_.architecture;
}

const CacheInfo& CPUDetector::getCacheInfo() const {
    return cpu_info_.cache_info;
}

const FrequencyInfo& CPUDetector::getFrequencyInfo() const {
    return cpu_info_.frequency_info;
}

const TopologyInfo& CPUDetector::getTopologyInfo() const {
    return cpu_info_.topology;
}

std::string CPUDetector::getFeatureString() const {
    if (!initialized_) {
        return "CPU features not detected";
    }
    
    std::stringstream ss;
    for (size_t i = 0; i < cpu_info_.features.size(); ++i) {
        if (i > 0) {
            ss << ", ";
        }
        ss << featureToString(cpu_info_.features[i]);
    }
    
    return ss.str();
}

std::string CPUDetector::getSummary() const {
    if (!initialized_) {
        return "CPU detection not initialized";
    }
    
    std::stringstream ss;
    ss << "CPU: " << cpu_info_.vendor << " " << cpu_info_.model_name << "\n";
    ss << "Architecture: " << static_cast<int>(cpu_info_.architecture) << "\n";
    ss << "Cores: " << cpu_info_.topology.physical_cores << " physical, " 
       << cpu_info_.topology.logical_cores << " logical\n";
    ss << "Sockets: " << cpu_info_.topology.sockets << "\n";
    ss << "NUMA Nodes: " << cpu_info_.topology.numa_nodes << "\n";
    ss << "Hyper-Threading: " << (cpu_info_.topology.hyper_threading ? "Yes" : "No") << "\n";
    ss << "Frequency: " << cpu_info_.frequency_info.base_frequency_mhz << " MHz (Base), " 
       << cpu_info_.frequency_info.max_frequency_mhz << " MHz (Max)\n";
    ss << "Cache: L1d=" << cpu_info_.cache_info.l1d_size_kb << "KB, L1i=" << cpu_info_.cache_info.l1i_size_kb 
       << "KB, L2=" << cpu_info_.cache_info.l2_size_kb << "KB, L3=" << cpu_info_.cache_info.l3_size_kb << "KB\n";
    ss << "Features: " << getFeatureString() << "\n";
    
    return ss.str();
}

bool CPUDetector::detectArchitecture() {
#if defined(__x86_64__) || defined(_M_X64)
    cpu_info_.architecture = CPUArchitecture::X86_64;
#elif defined(__i386__) || defined(_M_IX86)
    cpu_info_.architecture = CPUArchitecture::X86;
#elif defined(__aarch64__) || defined(_M_ARM64)
    cpu_info_.architecture = CPUArchitecture::ARM64;
#elif defined(__arm__) || defined(_M_ARM)
    cpu_info_.architecture = CPUArchitecture::ARM;
#elif defined(__powerpc64__)
    cpu_info_.architecture = CPUArchitecture::PPC64;
#elif defined(__powerpc__)
    cpu_info_.architecture = CPUArchitecture::PPC;
#else
    cpu_info_.architecture = CPUArchitecture::UNKNOWN;
#endif

    return cpu_info_.architecture != CPUArchitecture::UNKNOWN;
}

bool CPUDetector::detectVendorAndModel() {
#if defined(_WIN32) && (defined(_M_IX86) || defined(_M_X64))
    // Windows x86/x64
    int cpu_info[4] = {-1};
    char vendor[12];
    
    // Get vendor
    __cpuid(cpu_info, 0);
    memcpy(vendor, &cpu_info[1], 4);
    memcpy(vendor + 4, &cpu_info[3], 4);
    memcpy(vendor + 8, &cpu_info[2], 4);
    cpu_info_.vendor = std::string(vendor, 12);
    
    // Get brand string
    char brand[48];
    __cpuid(cpu_info, 0x80000000);
    unsigned int extended_ids = cpu_info[0];
    
    if (extended_ids >= 0x80000004) {
        memset(brand, 0, sizeof(brand));
        
        for (unsigned int i = 0x80000002; i <= 0x80000004; ++i) {
            __cpuid(cpu_info, i);
            memcpy(brand + (i - 0x80000002) * 16, cpu_info, 16);
        }
        
        cpu_info_.model_name = brand;
    } else {
        cpu_info_.model_name = "Unknown";
    }
    
    // Get family/model/stepping
    __cpuid(cpu_info, 1);
    cpu_info_.family = std::to_string((cpu_info[0] >> 8) & 0xf);
    cpu_info_.model = std::to_string((cpu_info[0] >> 4) & 0xf);
    cpu_info_.stepping = std::to_string(cpu_info[0] & 0xf);
    
#elif defined(__APPLE__)
    // macOS
    char buffer[128];
    size_t size = sizeof(buffer);
    
    if (sysctlbyname("machdep.cpu.vendor", buffer, &size, nullptr, 0) == 0) {
        cpu_info_.vendor = buffer;
    } else {
        cpu_info_.vendor = "Unknown";
    }
    
    size = sizeof(buffer);
    if (sysctlbyname("machdep.cpu.brand_string", buffer, &size, nullptr, 0) == 0) {
        cpu_info_.model_name = buffer;
    } else {
        cpu_info_.model_name = "Unknown";
    }
    
    size = sizeof(buffer);
    if (sysctlbyname("machdep.cpu.family", buffer, &size, nullptr, 0) == 0) {
        cpu_info_.family = buffer;
    }
    
    size = sizeof(buffer);
    if (sysctlbyname("machdep.cpu.model", buffer, &size, nullptr, 0) == 0) {
        cpu_info_.model = buffer;
    }
    
    size = sizeof(buffer);
    if (sysctlbyname("machdep.cpu.stepping", buffer, &size, nullptr, 0) == 0) {
        cpu_info_.stepping = buffer;
    }
    
#else
    // Linux and other Unix-like systems
    std::ifstream cpuinfo("/proc/cpuinfo");
    if (!cpuinfo.is_open()) {
        return false;
    }
    
    std::string line;
    while (std::getline(cpuinfo, line)) {
        if (line.find("vendor_id") != std::string::npos && cpu_info_.vendor.empty()) {
            size_t pos = line.find(':');
            if (pos != std::string::npos) {
                cpu_info_.vendor = line.substr(pos + 2);
            }
        } else if (line.find("model name") != std::string::npos && cpu_info_.model_name.empty()) {
            size_t pos = line.find(':');
            if (pos != std::string::npos) {
                cpu_info_.model_name = line.substr(pos + 2);
            }
        } else if (line.find("cpu family") != std::string::npos && cpu_info_.family.empty()) {
            size_t pos = line.find(':');
            if (pos != std::string::npos) {
                cpu_info_.family = line.substr(pos + 2);
            }
        } else if (line.find("model") != std::string::npos && line.find("model name") == std::string::npos && cpu_info_.model.empty()) {
            size_t pos = line.find(':');
            if (pos != std::string::npos) {
                cpu_info_.model = line.substr(pos + 2);
            }
        } else if (line.find("stepping") != std::string::npos && cpu_info_.stepping.empty()) {
            size_t pos = line.find(':');
            if (pos != std::string::npos) {
                cpu_info_.stepping = line.substr(pos + 2);
            }
        }
    }
#endif

    // Trim whitespace
    auto trim = [](std::string& s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
            return !std::isspace(ch);
        }));
        s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
            return !std::isspace(ch);
        }).base(), s.end());
    };
    
    trim(cpu_info_.vendor);
    trim(cpu_info_.model_name);
    trim(cpu_info_.family);
    trim(cpu_info_.model);
    trim(cpu_info_.stepping);
    
    return !cpu_info_.vendor.empty() && !cpu_info_.model_name.empty();
}

bool CPUDetector::detectTopology() {
    // Initialize with hardware concurrency as a fallback
    cpu_info_.topology.logical_cores = std::thread::hardware_concurrency();
    cpu_info_.topology.physical_cores = cpu_info_.topology.logical_cores;
    cpu_info_.topology.sockets = 1;
    cpu_info_.topology.cores_per_socket = cpu_info_.topology.physical_cores;
    cpu_info_.topology.hyper_threading = false;
    
#if defined(_WIN32)
    // Windows
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    cpu_info_.topology.logical_cores = sysInfo.dwNumberOfProcessors;
    
    // Get physical core count using GetLogicalProcessorInformation
    DWORD length = 0;
    GetLogicalProcessorInformation(nullptr, &length);
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> buffer(length / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
        if (GetLogicalProcessorInformation(&buffer[0], &length)) {
            int physical_cores = 0;
            int cache_l1 = 0, cache_l2 = 0, cache_l3 = 0;
            
            for (const auto& info : buffer) {
                if (info.Relationship == RelationProcessorCore) {
                    physical_cores++;
                } else if (info.Relationship == RelationCache) {
                    if (info.Cache.Level == 1 && info.Cache.Type == CacheData) {
                        cache_l1 += info.Cache.Size / 1024; // Convert to KB
                    } else if (info.Cache.Level == 2) {
                        cache_l2 += info.Cache.Size / 1024;
                    } else if (info.Cache.Level == 3) {
                        cache_l3 += info.Cache.Size / 1024;
                    }
                }
            }
            
            if (physical_cores > 0) {
                cpu_info_.topology.physical_cores = physical_cores;
                cpu_info_.topology.hyper_threading = (cpu_info_.topology.logical_cores > physical_cores);
            }
            
            // Update cache info
            cpu_info_.cache_info.l1d_size_kb = cache_l1;
            cpu_info_.cache_info.l2_size_kb = cache_l2;
            cpu_info_.cache_info.l3_size_kb = cache_l3;
        }
    }
    
#elif defined(__APPLE__)
    // macOS
    int physical_cores = 0;
    size_t len = sizeof(physical_cores);
    if (sysctlbyname("hw.physicalcpu", &physical_cores, &len, nullptr, 0) == 0) {
        cpu_info_.topology.physical_cores = physical_cores;
    }
    
    int logical_cores = 0;
    len = sizeof(logical_cores);
    if (sysctlbyname("hw.logicalcpu", &logical_cores, &len, nullptr, 0) == 0) {
        cpu_info_.topology.logical_cores = logical_cores;
    }
    
    cpu_info_.topology.hyper_threading = (logical_cores > physical_cores);
    
    // Try to get socket count
    int packages = 0;
    len = sizeof(packages);
    if (sysctlbyname("hw.packages", &packages, &len, nullptr, 0) == 0 && packages > 0) {
        cpu_info_.topology.sockets = packages;
    }
    
    // Calculate cores per socket
    if (cpu_info_.topology.sockets > 0) {
        cpu_info_.topology.cores_per_socket = cpu_info_.topology.physical_cores / cpu_info_.topology.sockets;
    }
    
    // Get cache information
    int l1d_size = 0, l1i_size = 0, l2_size = 0, l3_size = 0;
    len = sizeof(l1d_size);
    if (sysctlbyname("hw.l1dcachesize", &l1d_size, &len, nullptr, 0) == 0) {
        cpu_info_.cache_info.l1d_size_kb = l1d_size / 1024;
    }
    
    len = sizeof(l1i_size);
    if (sysctlbyname("hw.l1icachesize", &l1i_size, &len, nullptr, 0) == 0) {
        cpu_info_.cache_info.l1i_size_kb = l1i_size / 1024;
    }
    
    len = sizeof(l2_size);
    if (sysctlbyname("hw.l2cachesize", &l2_size, &len, nullptr, 0) == 0) {
        cpu_info_.cache_info.l2_size_kb = l2_size / 1024;
    }
    
    len = sizeof(l3_size);
    if (sysctlbyname("hw.l3cachesize", &l3_size, &len, nullptr, 0) == 0) {
        cpu_info_.cache_info.l3_size_kb = l3_size / 1024;
    }
    
#else
    // Linux and other Unix-like systems
    std::ifstream cpuinfo("/proc/cpuinfo");
    if (cpuinfo.is_open()) {
        std::string line;
        std::set<int> physical_ids;
        std::set<std::pair<int, int>> core_ids; // (physical_id, core_id) pairs
        
        int current_physical_id = -1;
        int current_core_id = -1;
        
        while (std::getline(cpuinfo, line)) {
            if (line.find("physical id") != std::string::npos) {
                size_t pos = line.find(':');
                if (pos != std::string::npos) {
                    current_physical_id = std::stoi(line.substr(pos + 2));
                    physical_ids.insert(current_physical_id);
                }
            } else if (line.find("core id") != std::string::npos) {
                size_t pos = line.find(':');
                if (pos != std::string::npos) {
                    current_core_id = std::stoi(line.substr(pos + 2));
                    if (current_physical_id >= 0) {
                        core_ids.insert(std::make_pair(current_physical_id, current_core_id));
                    }
                }
            } else if (line.empty()) {
                // Reset for next CPU
                current_physical_id = -1;
                current_core_id = -1;
            }
        }
        
        if (!physical_ids.empty()) {
            cpu_info_.topology.sockets = physical_ids.size();
        }
        
        if (!core_ids.empty()) {
            cpu_info_.topology.physical_cores = core_ids.size();
            cpu_info_.topology.hyper_threading = (cpu_info_.topology.logical_cores > cpu_info_.topology.physical_cores);
            
            if (cpu_info_.topology.sockets > 0) {
                cpu_info_.topology.cores_per_socket = cpu_info_.topology.physical_cores / cpu_info_.topology.sockets;
            }
        }
    }
    
    // Try to get NUMA information
    std::ifstream numa_nodes("/sys/devices/system/node/online");
    if (numa_nodes.is_open()) {
        std::string line;
        if (std::getline(numa_nodes, line)) {
            // Parse range like "0-3" or just "0"
            size_t dash_pos = line.find('-');
            if (dash_pos != std::string::npos) {
                int start = std::stoi(line.substr(0, dash_pos));
                int end = std::stoi(line.substr(dash_pos + 1));
                cpu_info_.topology.numa_nodes = end - start + 1;
            } else {
                // Just one number
                cpu_info_.topology.numa_nodes = 1;
            }
        }
    }
    
    // Get NUMA CPU mapping
    cpu_info_.topology.numa_cpu_mapping.resize(cpu_info_.topology.numa_nodes);
    for (int node = 0; node < cpu_info_.topology.numa_nodes; ++node) {
        std::string path = "/sys/devices/system/node/node" + std::to_string(node) + "/cpulist";
        std::ifstream cpulist(path);
        if (cpulist.is_open()) {
            std::string line;
            if (std::getline(cpulist, line)) {
                // Parse CPU list like "0-3,8-11" or just "0,1,2,3"
                std::stringstream ss(line);
                std::string range;
                while (std::getline(ss, range, ',')) {
                    size_t dash_pos = range.find('-');
                    if (dash_pos != std::string::npos) {
                        int start = std::stoi(range.substr(0, dash_pos));
                        int end = std::stoi(range.substr(dash_pos + 1));
                        for (int cpu = start; cpu <= end; ++cpu) {
                            cpu_info_.topology.numa_cpu_mapping[node].push_back(cpu);
                        }
                    } else {
                        // Just one number
                        cpu_info_.topology.numa_cpu_mapping[node].push_back(std::stoi(range));
                    }
                }
            }
        }
    }
    
    // Get cache information from sysfs
    auto readSysfsCacheInfo = [](int level, const std::string& type) -> int {
        std::string path = "/sys/devices/system/cpu/cpu0/cache/index" + std::to_string(level) + "/" + type;
        std::ifstream file(path);
        if (file.is_open()) {
            std::string line;
            if (std::getline(file, line)) {
                try {
                    return std::stoi(line);
                } catch (...) {
                    return 0;
                }
            }
        }
        return 0;
    };
    
    // L1 data cache
    int l1d_size = readSysfsCacheInfo(0, "size");
    if (l1d_size > 0) {
        // Convert from KB if needed
        if (l1d_size > 0) {
            std::string size_str = std::to_string(l1d_size);
            if (size_str.find('K') != std::string::npos) {
                l1d_size = std::stoi(size_str.substr(0, size_str.find('K')));
            } else if (size_str.find('M') != std::string::npos) {
                l1d_size = std::stoi(size_str.substr(0, size_str.find('M'))) * 1024;
            }
        }
        cpu_info_.cache_info.l1d_size_kb = l1d_size;
        cpu_info_.cache_info.l1d_line_size = readSysfsCacheInfo(0, "coherency_line_size");
        cpu_info_.cache_info.l1d_associativity = readSysfsCacheInfo(0, "ways_of_associativity");
    }
    
    // L1 instruction cache
    int l1i_size = readSysfsCacheInfo(1, "size");
    if (l1i_size > 0) {
        // Convert from KB if needed
        std::string size_str = std::to_string(l1i_size);
        if (size_str.find('K') != std::string::npos) {
            l1i_size = std::stoi(size_str.substr(0, size_str.find('K')));
        } else if (size_str.find('M') != std::string::npos) {
            l1i_size = std::stoi(size_str.substr(0, size_str.find('M'))) * 1024;
        }
        cpu_info_.cache_info.l1i_size_kb = l1i_size;
        cpu_info_.cache_info.l1i_line_size = readSysfsCacheInfo(1, "coherency_line_size");
        cpu_info_.cache_info.l1i_associativity = readSysfsCacheInfo(1, "ways_of_associativity");
    }
    
    // L2 cache
    int l2_size = readSysfsCacheInfo(2, "size");
    if (l2_size > 0) {
        // Convert from KB if needed
        std::string size_str = std::to_string(l2_size);
        if (size_str.find('K') != std::string::npos) {
            l2_size = std::stoi(size_str.substr(0, size_str.find('K')));
        } else if (size_str.find('M') != std::string::npos) {
            l2_size = std::stoi(size_str.substr(0, size_str.find('M'))) * 1024;
        }
        cpu_info_.cache_info.l2_size_kb = l2_size;
        cpu_info_.cache_info.l2_line_size = readSysfsCacheInfo(2, "coherency_line_size");
        cpu_info_.cache_info.l2_associativity = readSysfsCacheInfo(2, "ways_of_associativity");
    }
    
    // L3 cache
    int l3_size = readSysfsCacheInfo(3, "size");
    if (l3_size > 0) {
        // Convert from KB if needed
        std::string size_str = std::to_string(l3_size);
        if (size_str.find('K') != std::string::npos) {
            l3_size = std::stoi(size_str.substr(0, size_str.find('K')));
        } else if (size_str.find('M') != std::string::npos) {
            l3_size = std::stoi(size_str.substr(0, size_str.find('M'))) * 1024;
        }
        cpu_info_.cache_info.l3_size_kb = l3_size;
        cpu_info_.cache_info.l3_line_size = readSysfsCacheInfo(3, "coherency_line_size");
        cpu_info_.cache_info.l3_associativity = readSysfsCacheInfo(3, "ways_of_associativity");
    }
#endif

    return true;
}

bool CPUDetector::detectCPUFeatures() {
    cpu_info_.features.clear();
    
#if defined(_WIN32) && (defined(_M_IX86) || defined(_M_X64))
    // Windows x86/x64
    int cpu_info[4] = {-1};
    
    // Get standard features
    __cpuid(cpu_info, 1);
    
    if (cpu_info[3] & (1 << 25)) cpu_info_.features.push_back(CPUFeature::SSE);
    if (cpu_info[3] & (1 << 26)) cpu_info_.features.push_back(CPUFeature::SSE2);
    if (cpu_info[2] & (1 << 0))  cpu_info_.features.push_back(CPUFeature::SSE3);
    if (cpu_info[2] & (1 << 9))  cpu_info_.features.push_back(CPUFeature::SSSE3);
    if (cpu_info[2] & (1 << 19)) cpu_info_.features.push_back(CPUFeature::SSE41);
    if (cpu_info[2] & (1 << 20)) cpu_info_.features.push_back(CPUFeature::SSE42);
    if (cpu_info[2] & (1 << 28)) cpu_info_.features.push_back(CPUFeature::AVX);
    if (cpu_info[2] & (1 << 12)) cpu_info_.features.push_back(CPUFeature::FMA);
    if (cpu_info[2] & (1 << 25)) cpu_info_.features.push_back(CPUFeature::AES);
    if (cpu_info[2] & (1 << 23)) cpu_info_.features.push_back(CPUFeature::POPCNT);
    if (cpu_info[2] & (1 << 30)) cpu_info_.features.push_back(CPUFeature::RDRAND);
    
    // Get extended features
    __cpuid(cpu_info, 7);
    
    if (cpu_info[1] & (1 << 5))  cpu_info_.features.push_back(CPUFeature::AVX2);
    if (cpu_info[1] & (1 << 3))  cpu_info_.features.push_back(CPUFeature::BMI1);
    if (cpu_info[1] & (1 << 8))  cpu_info_.features.push_back(CPUFeature::BMI2);
    if (cpu_info[1] & (1 << 16)) cpu_info_.features.push_back(CPUFeature::AVX512F);
    if (cpu_info[1] & (1 << 17)) cpu_info_.features.push_back(CPUFeature::AVX512DQ);
    if (cpu_info[1] & (1 << 30)) cpu_info_.features.push_back(CPUFeature::AVX512BW);
    if (cpu_info[1] & (1 << 31)) cpu_info_.features.push_back(CPUFeature::AVX512VL);
    if (cpu_info[1] & (1 << 28)) cpu_info_.features.push_back(CPUFeature::AVX512CD);
    if (cpu_info[1] & (1 << 19)) cpu_info_.features.push_back(CPUFeature::ADX);
    if (cpu_info[1] & (1 << 29)) cpu_info_.features.push_back(CPUFeature::SHA);
    if (cpu_info[1] & (1 << 18)) cpu_info_.features.push_back(CPUFeature::RDSEED);
    if (cpu_info[1] & (1 << 0))  cpu_info_.features.push_back(CPUFeature::PREFETCHW);
    
    // Get extended features from extended function
    __cpuid(cpu_info, 0x80000001);
    
    if (cpu_info[2] & (1 << 5)) cpu_info_.features.push_back(CPUFeature::LZCNT);
    if (cpu_info[2] & (1 << 16)) cpu_info_.features.push_back(CPUFeature::F16C);
    
#elif defined(__APPLE__)
    // macOS
    char buffer[128];
    size_t size = sizeof(buffer);
    
    if (sysctlbyname("machdep.cpu.features", buffer, &size, nullptr, 0) == 0) {
        std::string features = buffer;
        std::transform(features.begin(), features.end(), features.begin(), ::toupper);
        
        if (features.find("SSE") != std::string::npos) cpu_info_.features.push_back(CPUFeature::SSE);
        if (features.find("SSE2") != std::string::npos) cpu_info_.features.push_back(CPUFeature::SSE2);
        if (features.find("SSE3") != std::string::npos) cpu_info_.features.push_back(CPUFeature::SSE3);
        if (features.find("SSSE3") != std::string::npos) cpu_info_.features.push_back(CPUFeature::SSSE3);
        if (features.find("SSE4.1") != std::string::npos) cpu_info_.features.push_back(CPUFeature::SSE41);
        if (features.find("SSE4.2") != std::string::npos) cpu_info_.features.push_back(CPUFeature::SSE42);
        if (features.find("AVX") != std::string::npos) cpu_info_.features.push_back(CPUFeature::AVX);
        if (features.find("AVX2") != std::string::npos) cpu_info_.features.push_back(CPUFeature::AVX2);
        if (features.find("FMA") != std::string::npos) cpu_info_.features.push_back(CPUFeature::FMA);
        if (features.find("BMI1") != std::string::npos) cpu_info_.features.push_back(CPUFeature::BMI1);
        if (features.find("BMI2") != std::string::npos) cpu_info_.features.push_back(CPUFeature::BMI2);
        if (features.find("AES") != std::string::npos) cpu_info_.features.push_back(CPUFeature::AES);
        if (features.find("POPCNT") != std::string::npos) cpu_info_.features.push_back(CPUFeature::POPCNT);
        if (features.find("LZCNT") != std::string::npos) cpu_info_.features.push_back(CPUFeature::LZCNT);
        if (features.find("F16C") != std::string::npos) cpu_info_.features.push_back(CPUFeature::F16C);
    }
    
    // Check for AVX512 features
    size = sizeof(buffer);
    if (sysctlbyname("machdep.cpu.leaf7_features", buffer, &size, nullptr, 0) == 0) {
        std::string features = buffer;
        std::transform(features.begin(), features.end(), features.begin(), ::toupper);
        
        if (features.find("AVX512F") != std::string::npos) cpu_info_.features.push_back(CPUFeature::AVX512F);
        if (features.find("AVX512DQ") != std::string::npos) cpu_info_.features.push_back(CPUFeature::AVX512DQ);
        if (features.find("AVX512BW") != std::string::npos) cpu_info_.features.push_back(CPUFeature::AVX512BW);
        if (features.find("AVX512VL") != std::string::npos) cpu_info_.features.push_back(CPUFeature::AVX512VL);
        if (features.find("AVX512CD") != std::string::npos) cpu_info_.features.push_back(CPUFeature::AVX512CD);
        if (features.find("SHA") != std::string::npos) cpu_info_.features.push_back(CPUFeature::SHA);
        if (features.find("RDSEED") != std::string::npos) cpu_info_.features.push_back(CPUFeature::RDSEED);
        if (features.find("ADX") != std::string::npos) cpu_info_.features.push_back(CPUFeature::ADX);
        if (features.find("PREFETCHW") != std::string::npos) cpu_info_.features.push_back(CPUFeature::PREFETCHW);
    }
    
#else
    // Linux and other Unix-like systems
    std::ifstream cpuinfo("/proc/cpuinfo");
    if (!cpuinfo.is_open()) {
        return false;
    }
    
    std::string line;
    while (std::getline(cpuinfo, line)) {
        if (line.find("flags") != std::string::npos) {
            size_t pos = line.find(':');
            if (pos != std::string::npos) {
                std::string flags = line.substr(pos + 2);
                parseCPUFlags(flags);
                break; // Only need to parse flags once
            }
        }
    }
#endif

    return !cpu_info_.features.empty();
}

bool CPUDetector::parseCPUFlags(const std::string& flags) {
    std::stringstream ss(flags);
    std::string flag;
    
    while (ss >> flag) {
        if (flag == "sse") cpu_info_.features.push_back(CPUFeature::SSE);
        else if (flag == "sse2") cpu_info_.features.push_back(CPUFeature::SSE2);
        else if (flag == "pni" || flag == "sse3") cpu_info_.features.push_back(CPUFeature::SSE3);
        else if (flag == "ssse3") cpu_info_.features.push_back(CPUFeature::SSSE3);
        else if (flag == "sse4_1") cpu_info_.features.push_back(CPUFeature::SSE41);
        else if (flag == "sse4_2") cpu_info_.features.push_back(CPUFeature::SSE42);
        else if (flag == "avx") cpu_info_.features.push_back(CPUFeature::AVX);
        else if (flag == "avx2") cpu_info_.features.push_back(CPUFeature::AVX2);
        else if (flag == "fma") cpu_info_.features.push_back(CPUFeature::FMA);
        else if (flag == "bmi1") cpu_info_.features.push_back(CPUFeature::BMI1);
        else if (flag == "bmi2") cpu_info_.features.push_back(CPUFeature::BMI2);
        else if (flag == "aes") cpu_info_.features.push_back(CPUFeature::AES);
        else if (flag == "popcnt") cpu_info_.features.push_back(CPUFeature::POPCNT);
        else if (flag == "lzcnt") cpu_info_.features.push_back(CPUFeature::LZCNT);
        else if (flag == "f16c") cpu_info_.features.push_back(CPUFeature::F16C);
        else if (flag == "avx512f") cpu_info_.features.push_back(CPUFeature::AVX512F);
        else if (flag == "avx512dq") cpu_info_.features.push_back(CPUFeature::AVX512DQ);
        else if (flag == "avx512bw") cpu_info_.features.push_back(CPUFeature::AVX512BW);
        else if (flag == "avx512vl") cpu_info_.features.push_back(CPUFeature::AVX512VL);
        else if (flag == "avx512cd") cpu_info_.features.push_back(CPUFeature::AVX512CD);
        else if (flag == "sha_ni" || flag == "sha") cpu_info_.features.push_back(CPUFeature::SHA);
        else if (flag == "rdseed") cpu_info_.features.push_back(CPUFeature::RDSEED);
        else if (flag == "adx") cpu_info_.features.push_back(CPUFeature::ADX);
        else if (flag == "prefetchw") cpu_info_.features.push_back(CPUFeature::PREFETCHW);
    }
    
    return !cpu_info_.features.empty();
}

bool CPUDetector::detectFrequencyInfo() {
#if defined(_WIN32)
    // Windows
    HKEY hKey;
    DWORD dwMHz = 0;
    DWORD dwSize = sizeof(dwMHz);
    
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueEx(hKey, "~MHz", NULL, NULL, (LPBYTE)&dwMHz, &dwSize) == ERROR_SUCCESS) {
            cpu_info_.frequency_info.base_frequency_mhz = static_cast<double>(dwMHz);
        }
        RegCloseKey(hKey);
    }
    
    // Try to get max frequency from WMI
    std::string output;
    if (executeCommand("wmic cpu get MaxClockSpeed /value", output)) {
        size_t pos = output.find("=");
        if (pos != std::string::npos) {
            try {
                cpu_info_.frequency_info.max_frequency_mhz = std::stod(output.substr(pos + 1));
            } catch (...) {
                // Use base frequency as fallback
                cpu_info_.frequency_info.max_frequency_mhz = cpu_info_.frequency_info.base_frequency_mhz;
            }
        }
    } else {
        // Use base frequency as fallback
        cpu_info_.frequency_info.max_frequency_mhz = cpu_info_.frequency_info.base_frequency_mhz;
    }
    
    // Check for turbo boost
    cpu_info_.frequency_info.turbo_boost = (cpu_info_.frequency_info.max_frequency_mhz > cpu_info_.frequency_info.base_frequency_mhz);
    
#elif defined(__APPLE__)
    // macOS
    int64_t freq = 0;
    size_t size = sizeof(freq);
    
    if (sysctlbyname("hw.cpufrequency", &freq, &size, nullptr, 0) == 0) {
        cpu_info_.frequency_info.base_frequency_mhz = static_cast<double>(freq) / 1000000.0;
    }
    
    size = sizeof(freq);
    if (sysctlbyname("hw.cpufrequency_max", &freq, &size, nullptr, 0) == 0) {
        cpu_info_.frequency_info.max_frequency_mhz = static_cast<double>(freq) / 1000000.0;
    } else {
        cpu_info_.frequency_info.max_frequency_mhz = cpu_info_.frequency_info.base_frequency_mhz;
    }
    
    size = sizeof(freq);
    if (sysctlbyname("hw.cpufrequency_min", &freq, &size, nullptr, 0) == 0) {
        cpu_info_.frequency_info.min_frequency_mhz = static_cast<double>(freq) / 1000000.0;
    } else {
        cpu_info_.frequency_info.min_frequency_mhz = cpu_info_.frequency_info.base_frequency_mhz;
    }
    
    // Check for turbo boost
    cpu_info_.frequency_info.turbo_boost = (cpu_info_.frequency_info.max_frequency_mhz > cpu_info_.frequency_info.base_frequency_mhz);
    
#else
    // Linux and other Unix-like systems
    
    // Try to get base frequency from cpuinfo
    std::ifstream cpuinfo("/proc/cpuinfo");
    if (cpuinfo.is_open()) {
        std::string line;
        while (std::getline(cpuinfo, line)) {
            if (line.find("cpu MHz") != std::string::npos) {
                size_t pos = line.find(':');
                if (pos != std::string::npos) {
                    try {
                        cpu_info_.frequency_info.base_frequency_mhz = std::stod(line.substr(pos + 2));
                        break;
                    } catch (...) {
                        // Ignore parsing errors
                    }
                }
            }
        }
    }
    
    // Try to get min/max frequency from cpufreq
    std::ifstream max_freq("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
    if (max_freq.is_open()) {
        std::string line;
        if (std::getline(max_freq, line)) {
            try {
                // Convert from kHz to MHz
                cpu_info_.frequency_info.max_frequency_mhz = std::stod(line) / 1000.0;
            } catch (...) {
                // Use base frequency as fallback
                cpu_info_.frequency_info.max_frequency_mhz = cpu_info_.frequency_info.base_frequency_mhz;
            }
        }
    } else {
        // Use base frequency as fallback
        cpu_info_.frequency_info.max_frequency_mhz = cpu_info_.frequency_info.base_frequency_mhz;
    }
    
    std::ifstream min_freq("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_min_freq");
    if (min_freq.is_open()) {
        std::string line;
        if (std::getline(min_freq, line)) {
            try {
                // Convert from kHz to MHz
                cpu_info_.frequency_info.min_frequency_mhz = std::stod(line) / 1000.0;
            } catch (...) {
                // Use base frequency as fallback
                cpu_info_.frequency_info.min_frequency_mhz = cpu_info_.frequency_info.base_frequency_mhz;
            }
        }
    } else {
        // Use base frequency as fallback
        cpu_info_.frequency_info.min_frequency_mhz = cpu_info_.frequency_info.base_frequency_mhz;
    }
    
    // Check for turbo boost
    cpu_info_.frequency_info.turbo_boost = (cpu_info_.frequency_info.max_frequency_mhz > cpu_info_.frequency_info.base_frequency_mhz);
    
    // Try to get turbo boost max frequency
    std::ifstream turbo_freq("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
    if (turbo_freq.is_open()) {
        std::string line;
        if (std::getline(turbo_freq, line)) {
            try {
                // Convert from kHz to MHz
                cpu_info_.frequency_info.max_turbo_frequency_mhz = std::stoi(line) / 1000;
            } catch (...) {
                // Use max frequency as fallback
                cpu_info_.frequency_info.max_turbo_frequency_mhz = static_cast<int>(cpu_info_.frequency_info.max_frequency_mhz);
            }
        }
    } else {
        // Use max frequency as fallback
        cpu_info_.frequency_info.max_turbo_frequency_mhz = static_cast<int>(cpu_info_.frequency_info.max_frequency_mhz);
    }
#endif

    return cpu_info_.frequency_info.base_frequency_mhz > 0;
}

bool CPUDetector::detectCacheInfo() {
    // Most of the cache detection is done in detectTopology()
    // This function is mainly for additional cache information
    
    // If we already have cache information, return true
    if (cpu_info_.cache_info.l1d_size_kb > 0 || cpu_info_.cache_info.l2_size_kb > 0 || cpu_info_.cache_info.l3_size_kb > 0) {
        return true;
    }
    
    // Otherwise, try to detect cache information
#if defined(_WIN32)
    // Windows
    // Cache detection is done in detectTopology()
    
#elif defined(__APPLE__)
    // macOS
    // Cache detection is done in detectTopology()
    
#else
    // Linux and other Unix-like systems
    // Try to get cache information from sysfs
    auto readSysfsCacheInfo = [](int level, const std::string& type) -> int {
        std::string path = "/sys/devices/system/cpu/cpu0/cache/index" + std::to_string(level) + "/" + type;
        std::ifstream file(path);
        if (file.is_open()) {
            std::string line;
            if (std::getline(file, line)) {
                try {
                    return std::stoi(line);
                } catch (...) {
                    return 0;
                }
            }
        }
        return 0;
    };
    
    // Try to find the correct cache index for each level
    for (int i = 0; i < 4; ++i) {
        std::string path = "/sys/devices/system/cpu/cpu0/cache/index" + std::to_string(i) + "/level";
        std::ifstream file(path);
        if (file.is_open()) {
            std::string line;
            if (std::getline(file, line)) {
                int level = std::stoi(line);
                
                // Check cache type
                std::string type_path = "/sys/devices/system/cpu/cpu0/cache/index" + std::to_string(i) + "/type";
                std::ifstream type_file(type_path);
                std::string cache_type;
                if (type_file.is_open() && std::getline(type_file, cache_type)) {
                    // Get cache size
                    std::string size_path = "/sys/devices/system/cpu/cpu0/cache/index" + std::to_string(i) + "/size";
                    std::ifstream size_file(size_path);
                    std::string size_str;
                    if (size_file.is_open() && std::getline(size_file, size_str)) {
                        int size_kb = 0;
                        if (size_str.find('K') != std::string::npos) {
                            size_kb = std::stoi(size_str.substr(0, size_str.find('K')));
                        } else if (size_str.find('M') != std::string::npos) {
                            size_kb = std::stoi(size_str.substr(0, size_str.find('M'))) * 1024;
                        } else {
                            try {
                                size_kb = std::stoi(size_str);
                            } catch (...) {
                                size_kb = 0;
                            }
                        }
                        
                        // Get line size and associativity
                        int line_size = readSysfsCacheInfo(i, "coherency_line_size");
                        int associativity = readSysfsCacheInfo(i, "ways_of_associativity");
                        
                        // Update cache info
                        if (level == 1 && cache_type == "Data") {
                            cpu_info_.cache_info.l1d_size_kb = size_kb;
                            cpu_info_.cache_info.l1d_line_size = line_size;
                            cpu_info_.cache_info.l1d_associativity = associativity;
                        } else if (level == 1 && cache_type == "Instruction") {
                            cpu_info_.cache_info.l1i_size_kb = size_kb;
                            cpu_info_.cache_info.l1i_line_size = line_size;
                            cpu_info_.cache_info.l1i_associativity = associativity;
                        } else if (level == 2) {
                            cpu_info_.cache_info.l2_size_kb = size_kb;
                            cpu_info_.cache_info.l2_line_size = line_size;
                            cpu_info_.cache_info.l2_associativity = associativity;
                        } else if (level == 3) {
                            cpu_info_.cache_info.l3_size_kb = size_kb;
                            cpu_info_.cache_info.l3_line_size = line_size;
                            cpu_info_.cache_info.l3_associativity = associativity;
                        }
                    }
                }
            }
        }
    }
#endif

    return true;
}

bool CPUDetector::executeCommand(const std::string& command, std::string& output) {
    output.clear();
    
#if defined(_WIN32)
    // Windows
    HANDLE hReadPipe, hWritePipe;
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;
    
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
        return false;
    }
    
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(STARTUPINFO));
    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
    si.cb = sizeof(STARTUPINFO);
    si.hStdError = hWritePipe;
    si.hStdOutput = hWritePipe;
    si.dwFlags |= STARTF_USESTDHANDLES;
    
    if (!CreateProcess(NULL, const_cast<LPSTR>(command.c_str()), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return false;
    }
    
    CloseHandle(hWritePipe);
    
    char buffer[4096];
    DWORD bytesRead;
    while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        output += buffer;
    }
    
    CloseHandle(hReadPipe);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
    return true;
#else
    // Unix-like systems
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        return false;
    }
    
    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        output += buffer;
    }
    
    int status = pclose(pipe);
    return status == 0;
#endif
}

std::string CPUDetector::featureToString(CPUFeature feature) const {
    switch (feature) {
        case CPUFeature::SSE: return "SSE";
        case CPUFeature::SSE2: return "SSE2";
        case CPUFeature::SSE3: return "SSE3";
        case CPUFeature::SSSE3: return "SSSE3";
        case CPUFeature::SSE41: return "SSE4.1";
        case CPUFeature::SSE42: return "SSE4.2";
        case CPUFeature::AVX: return "AVX";
        case CPUFeature::AVX2: return "AVX2";
        case CPUFeature::AVX512F: return "AVX512F";
        case CPUFeature::AVX512BW: return "AVX512BW";
        case CPUFeature::AVX512CD: return "AVX512CD";
        case CPUFeature::AVX512DQ: return "AVX512DQ";
        case CPUFeature::AVX512VL: return "AVX512VL";
        case CPUFeature::FMA: return "FMA";
        case CPUFeature::BMI1: return "BMI1";
        case CPUFeature::BMI2: return "BMI2";
        case CPUFeature::AES: return "AES";
        case CPUFeature::SHA: return "SHA";
        case CPUFeature::RDRAND: return "RDRAND";
        case CPUFeature::RDSEED: return "RDSEED";
        case CPUFeature::ADX: return "ADX";
        case CPUFeature::PREFETCHW: return "PREFETCHW";
        case CPUFeature::F16C: return "F16C";
        case CPUFeature::POPCNT: return "POPCNT";
        case CPUFeature::LZCNT: return "LZCNT";
        default: return "Unknown";
    }
}

CPUFeature CPUDetector::stringToFeature(const std::string& feature_str) const {
    if (feature_str == "SSE") return CPUFeature::SSE;
    if (feature_str == "SSE2") return CPUFeature::SSE2;
    if (feature_str == "SSE3") return CPUFeature::SSE3;
    if (feature_str == "SSSE3") return CPUFeature::SSSE3;
    if (feature_str == "SSE4.1") return CPUFeature::SSE41;
    if (feature_str == "SSE4.2") return CPUFeature::SSE42;
    if (feature_str == "AVX") return CPUFeature::AVX;
    if (feature_str == "AVX2") return CPUFeature::AVX2;
    if (feature_str == "AVX512F") return CPUFeature::AVX512F;
    if (feature_str == "AVX512BW") return CPUFeature::AVX512BW;
    if (feature_str == "AVX512CD") return CPUFeature::AVX512CD;
    if (feature_str == "AVX512DQ") return CPUFeature::AVX512DQ;
    if (feature_str == "AVX512VL") return CPUFeature::AVX512VL;
    if (feature_str == "FMA") return CPUFeature::FMA;
    if (feature_str == "BMI1") return CPUFeature::BMI1;
    if (feature_str == "BMI2") return CPUFeature::BMI2;
    if (feature_str == "AES") return CPUFeature::AES;
    if (feature_str == "SHA") return CPUFeature::SHA;
    if (feature_str == "RDRAND") return CPUFeature::RDRAND;
    if (feature_str == "RDSEED") return CPUFeature::RDSEED;
    if (feature_str == "ADX") return CPUFeature::ADX;
    if (feature_str == "PREFETCHW") return CPUFeature::PREFETCHW;
    if (feature_str == "F16C") return CPUFeature::F16C;
    if (feature_str == "POPCNT") return CPUFeature::POPCNT;
    if (feature_str == "LZCNT") return CPUFeature::LZCNT;
    
    // Default
    return CPUFeature::SSE; // Just return something
}

int CPUDetector::calculateOptimalThreadCount(WorkloadType workload) const {
    // Get the number of logical cores
    int logical_cores = cpu_info_.topology.logical_cores;
    
    // If we don't have topology information, use hardware concurrency
    if (logical_cores <= 0) {
        logical_cores = std::thread::hardware_concurrency();
    }
    
    // Default to using all logical cores
    int optimal_threads = logical_cores;
    
    // Adjust based on workload type
    switch (workload) {
        case WorkloadType::COMPUTE_INTENSIVE:
            // For compute-intensive workloads, use all logical cores
            optimal_threads = logical_cores;
            break;
            
        case WorkloadType::MEMORY_INTENSIVE:
            // For memory-intensive workloads, use physical cores to avoid cache thrashing
            if (cpu_info_.topology.physical_cores > 0) {
                optimal_threads = cpu_info_.topology.physical_cores;
            } else {
                // If we don't know physical cores, use 75% of logical cores
                optimal_threads = static_cast<int>(logical_cores * 0.75);
            }
            break;
            
        case WorkloadType::IO_INTENSIVE:
            // For I/O-intensive workloads, use more threads to hide I/O latency
            optimal_threads = logical_cores * 2;
            break;
            
        case WorkloadType::BALANCED:
            // For balanced workloads, use all logical cores
            optimal_threads = logical_cores;
            break;
    }
    
    // Ensure we have at least one thread
    if (optimal_threads < 1) {
        optimal_threads = 1;
    }
    
    return optimal_threads;
}

} // namespace system
} // namespace mfp
