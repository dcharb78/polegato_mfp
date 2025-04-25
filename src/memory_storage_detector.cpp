#include "memory_storage_detector.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <thread>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <random>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#include <sysinfoapi.h>
#include <psapi.h>
#elif defined(__APPLE__)
#include <sys/sysctl.h>
#include <mach/mach.h>
#include <mach/vm_statistics.h>
#include <mach/mach_types.h>
#include <mach/mach_init.h>
#include <mach/mach_host.h>
#include <sys/mount.h>
#include <sys/param.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <fcntl.h>
#include <sys/statvfs.h>
#include <dirent.h>
#include <numa.h>
#endif

namespace mfp {
namespace system {

//=============================================================================
// MemoryDetector Implementation
//=============================================================================

MemoryDetector::MemoryDetector() {
    // Initialize with default values
}

MemoryDetector::~MemoryDetector() {
    // Clean up resources if needed
}

bool MemoryDetector::detect() {
    if (initialized_) {
        return true; // Already initialized
    }
    
    bool success = true;
    
    // Detect basic memory information
    success &= detectTotalMemory();
    success &= detectAvailableMemory();
    success &= detectPageSize();
    success &= detectMemoryType();
    success &= detectMemorySpeed();
    success &= detectNUMATopology();
    success &= detectMemoryBandwidth();
    
    initialized_ = success;
    return success;
}

const MemoryInfo& MemoryDetector::getMemoryInfo() const {
    return memory_info_;
}

size_t MemoryDetector::getOptimalAllocationSize(WorkloadType workload) const {
    if (!initialized_) {
        // Default to 1GB if not initialized
        return 1024 * 1024 * 1024;
    }
    
    // Get available memory
    size_t available_memory = memory_info_.available_physical_memory_bytes;
    
    // Calculate optimal allocation size based on workload type
    switch (workload) {
        case WorkloadType::COMPUTE_INTENSIVE:
            // For compute-intensive workloads, use 50% of available memory
            return static_cast<size_t>(available_memory * 0.5);
            
        case WorkloadType::MEMORY_INTENSIVE:
            // For memory-intensive workloads, use 80% of available memory
            return static_cast<size_t>(available_memory * 0.8);
            
        case WorkloadType::IO_INTENSIVE:
            // For I/O-intensive workloads, use 30% of available memory for caching
            return static_cast<size_t>(available_memory * 0.3);
            
        case WorkloadType::BALANCED:
            // For balanced workloads, use 60% of available memory
            return static_cast<size_t>(available_memory * 0.6);
            
        default:
            // Default to 50% of available memory
            return static_cast<size_t>(available_memory * 0.5);
    }
}

bool MemoryDetector::hasNUMA() const {
    return memory_info_.numa_available;
}

int MemoryDetector::getNumaNodeCount() const {
    return memory_info_.numa_node_count;
}

double MemoryDetector::getMemoryBandwidth() const {
    return memory_info_.memory_bandwidth_gbps;
}

std::string MemoryDetector::getSummary() const {
    if (!initialized_) {
        return "Memory detection not initialized";
    }
    
    std::stringstream ss;
    ss << "Memory Information:\n";
    ss << "  Total Physical Memory: " << (memory_info_.total_physical_memory_bytes / (1024 * 1024 * 1024)) << " GB\n";
    ss << "  Available Physical Memory: " << (memory_info_.available_physical_memory_bytes / (1024 * 1024 * 1024)) << " GB\n";
    ss << "  Page Size: " << memory_info_.page_size_bytes << " bytes\n";
    ss << "  Memory Type: " << static_cast<int>(memory_info_.memory_type) << "\n";
    ss << "  Memory Speed: " << memory_info_.memory_speed_mhz << " MHz\n";
    ss << "  Memory Bandwidth: " << memory_info_.memory_bandwidth_gbps << " GB/s\n";
    ss << "  NUMA Available: " << (memory_info_.numa_available ? "Yes" : "No") << "\n";
    ss << "  NUMA Node Count: " << memory_info_.numa_node_count << "\n";
    
    if (memory_info_.numa_available) {
        ss << "  NUMA Nodes:\n";
        for (const auto& node : memory_info_.numa_nodes) {
            ss << "    Node " << node.node_id << ": " 
               << (node.memory_bytes / (1024 * 1024 * 1024)) << " GB, " 
               << node.cpu_cores.size() << " cores\n";
        }
    }
    
    return ss.str();
}

bool MemoryDetector::detectTotalMemory() {
#if defined(_WIN32)
    // Windows
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        memory_info_.total_physical_memory_bytes = static_cast<size_t>(memInfo.ullTotalPhys);
        memory_info_.total_virtual_memory_bytes = static_cast<size_t>(memInfo.ullTotalVirtual);
        return true;
    }
    return false;
    
#elif defined(__APPLE__)
    // macOS
    int64_t mem_size = 0;
    size_t len = sizeof(mem_size);
    if (sysctlbyname("hw.memsize", &mem_size, &len, NULL, 0) == 0) {
        memory_info_.total_physical_memory_bytes = static_cast<size_t>(mem_size);
        
        // Virtual memory is not directly available on macOS
        // Use a reasonable estimate (physical memory + swap)
        int64_t swap_size = 0;
        len = sizeof(swap_size);
        if (sysctlbyname("vm.swapusage", &swap_size, &len, NULL, 0) == 0) {
            memory_info_.total_virtual_memory_bytes = memory_info_.total_physical_memory_bytes + static_cast<size_t>(swap_size);
        } else {
            // If swap size is not available, estimate virtual memory as 2x physical memory
            memory_info_.total_virtual_memory_bytes = memory_info_.total_physical_memory_bytes * 2;
        }
        
        return true;
    }
    return false;
    
#else
    // Linux and other Unix-like systems
    struct sysinfo memInfo;
    if (sysinfo(&memInfo) == 0) {
        memory_info_.total_physical_memory_bytes = static_cast<size_t>(memInfo.totalram) * memInfo.mem_unit;
        memory_info_.total_virtual_memory_bytes = static_cast<size_t>(memInfo.totalram + memInfo.totalswap) * memInfo.mem_unit;
        return true;
    }
    return false;
#endif
}

bool MemoryDetector::detectAvailableMemory() {
#if defined(_WIN32)
    // Windows
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        memory_info_.available_physical_memory_bytes = static_cast<size_t>(memInfo.ullAvailPhys);
        memory_info_.available_virtual_memory_bytes = static_cast<size_t>(memInfo.ullAvailVirtual);
        return true;
    }
    return false;
    
#elif defined(__APPLE__)
    // macOS
    vm_statistics64_data_t vm_stats;
    mach_msg_type_number_t count = sizeof(vm_stats) / sizeof(natural_t);
    if (host_statistics64(mach_host_self(), HOST_VM_INFO64, (host_info64_t)&vm_stats, &count) == KERN_SUCCESS) {
        // Calculate free memory (free + inactive)
        size_t page_size = memory_info_.page_size_bytes;
        if (page_size == 0) {
            // If page size is not yet detected, get it now
            page_size = getpagesize();
            memory_info_.page_size_bytes = page_size;
        }
        
        size_t free_memory = (vm_stats.free_count + vm_stats.inactive_count) * page_size;
        memory_info_.available_physical_memory_bytes = free_memory;
        
        // Virtual memory is not directly available on macOS
        // Use a reasonable estimate (available physical memory + available swap)
        xsw_usage swap_usage;
        size_t len = sizeof(swap_usage);
        if (sysctlbyname("vm.swapusage", &swap_usage, &len, NULL, 0) == 0) {
            memory_info_.available_virtual_memory_bytes = memory_info_.available_physical_memory_bytes + swap_usage.xsu_avail;
        } else {
            // If swap usage is not available, estimate available virtual memory as available physical memory
            memory_info_.available_virtual_memory_bytes = memory_info_.available_physical_memory_bytes;
        }
        
        return true;
    }
    return false;
    
#else
    // Linux and other Unix-like systems
    struct sysinfo memInfo;
    if (sysinfo(&memInfo) == 0) {
        memory_info_.available_physical_memory_bytes = static_cast<size_t>(memInfo.freeram + memInfo.bufferram) * memInfo.mem_unit;
        memory_info_.available_virtual_memory_bytes = static_cast<size_t>(memInfo.freeram + memInfo.freeswap) * memInfo.mem_unit;
        
        // Try to get a more accurate available memory from /proc/meminfo
        std::ifstream meminfo("/proc/meminfo");
        if (meminfo.is_open()) {
            std::string line;
            size_t mem_available = 0;
            
            while (std::getline(meminfo, line)) {
                if (line.find("MemAvailable:") != std::string::npos) {
                    // Extract the value (in kB)
                    size_t pos = line.find(':');
                    if (pos != std::string::npos) {
                        std::string value = line.substr(pos + 1);
                        // Remove non-numeric characters
                        value.erase(std::remove_if(value.begin(), value.end(), [](char c) { return !std::isdigit(c); }), value.end());
                        try {
                            mem_available = std::stoull(value) * 1024; // Convert kB to bytes
                            memory_info_.available_physical_memory_bytes = mem_available;
                            break;
                        } catch (...) {
                            // Ignore parsing errors
                        }
                    }
                }
            }
        }
        
        return true;
    }
    return false;
#endif
}

bool MemoryDetector::detectPageSize() {
#if defined(_WIN32)
    // Windows
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    memory_info_.page_size_bytes = sysInfo.dwPageSize;
    return true;
    
#elif defined(__APPLE__) || defined(__linux__) || defined(__unix__) || defined(__FreeBSD__)
    // macOS, Linux, and other Unix-like systems
    memory_info_.page_size_bytes = getpagesize();
    return true;
    
#else
    // Default to 4KB page size
    memory_info_.page_size_bytes = 4096;
    return true;
#endif
}

bool MemoryDetector::detectMemoryType() {
    // Default to unknown
    memory_info_.memory_type = MemoryType::UNKNOWN;
    
#if defined(_WIN32)
    // Windows
    // Use WMI to get memory type
    std::string output;
    if (executeCommand("wmic memorychip get SMBIOSMemoryType", output)) {
        // Parse output to determine memory type
        // SMBIOSMemoryType values:
        // 26 = DDR4
        // 24 = DDR3
        // 34 = DDR5
        if (output.find("26") != std::string::npos) {
            memory_info_.memory_type = MemoryType::DDR4;
        } else if (output.find("24") != std::string::npos) {
            memory_info_.memory_type = MemoryType::DDR3;
        } else if (output.find("34") != std::string::npos) {
            memory_info_.memory_type = MemoryType::DDR5;
        }
    }
    
#elif defined(__APPLE__)
    // macOS
    // Try to determine memory type from system profiler
    std::string output;
    if (executeCommand("system_profiler SPMemoryDataType | grep 'Type:'", output)) {
        if (output.find("DDR5") != std::string::npos) {
            memory_info_.memory_type = MemoryType::DDR5;
        } else if (output.find("DDR4") != std::string::npos) {
            memory_info_.memory_type = MemoryType::DDR4;
        } else if (output.find("DDR3") != std::string::npos) {
            memory_info_.memory_type = MemoryType::DDR3;
        } else if (output.find("LPDDR5") != std::string::npos) {
            memory_info_.memory_type = MemoryType::LPDDR5;
        } else if (output.find("LPDDR4") != std::string::npos) {
            memory_info_.memory_type = MemoryType::LPDDR4;
        }
    }
    
#else
    // Linux and other Unix-like systems
    // Try to determine memory type from DMI information
    std::string output;
    if (executeCommand("sudo dmidecode -t memory | grep -i 'Type:'", output)) {
        if (output.find("DDR5") != std::string::npos) {
            memory_info_.memory_type = MemoryType::DDR5;
        } else if (output.find("DDR4") != std::string::npos) {
            memory_info_.memory_type = MemoryType::DDR4;
        } else if (output.find("DDR3") != std::string::npos) {
            memory_info_.memory_type = MemoryType::DDR3;
        } else if (output.find("LPDDR5") != std::string::npos) {
            memory_info_.memory_type = MemoryType::LPDDR5;
        } else if (output.find("LPDDR4") != std::string::npos) {
            memory_info_.memory_type = MemoryType::LPDDR4;
        }
    }
#endif

    // If we couldn't determine the memory type, try to make an educated guess
    if (memory_info_.memory_type == MemoryType::UNKNOWN) {
        // Check memory speed to guess type
        if (memory_info_.memory_speed_mhz > 4000) {
            memory_info_.memory_type = MemoryType::DDR5;
        } else if (memory_info_.memory_speed_mhz > 2400) {
            memory_info_.memory_type = MemoryType::DDR4;
        } else if (memory_info_.memory_speed_mhz > 1066) {
            memory_info_.memory_type = MemoryType::DDR3;
        }
    }
    
    return true; // Always return true since this is not critical
}

bool MemoryDetector::detectMemorySpeed() {
#if defined(_WIN32)
    // Windows
    // Use WMI to get memory speed
    std::string output;
    if (executeCommand("wmic memorychip get speed", output)) {
        // Parse output to get memory speed
        std::istringstream iss(output);
        std::string line;
        // Skip header line
        std::getline(iss, line);
        // Get first speed value
        if (std::getline(iss, line)) {
            // Remove non-numeric characters
            line.erase(std::remove_if(line.begin(), line.end(), [](char c) { return !std::isdigit(c); }), line.end());
            if (!line.empty()) {
                try {
                    memory_info_.memory_speed_mhz = std::stod(line);
                } catch (...) {
                    // Ignore parsing errors
                }
            }
        }
    }
    
#elif defined(__APPLE__)
    // macOS
    // Try to determine memory speed from system profiler
    std::string output;
    if (executeCommand("system_profiler SPMemoryDataType | grep 'Speed:'", output)) {
        // Extract numeric part
        size_t pos = output.find("MHz");
        if (pos != std::string::npos) {
            std::string speed_str = output.substr(0, pos);
            // Remove non-numeric characters
            speed_str.erase(std::remove_if(speed_str.begin(), speed_str.end(), [](char c) { return !std::isdigit(c); }), speed_str.end());
            if (!speed_str.empty()) {
                try {
                    memory_info_.memory_speed_mhz = std::stod(speed_str);
                } catch (...) {
                    // Ignore parsing errors
                }
            }
        }
    }
    
#else
    // Linux and other Unix-like systems
    // Try to determine memory speed from DMI information
    std::string output;
    if (executeCommand("sudo dmidecode -t memory | grep -i 'Speed:'", output)) {
        // Extract numeric part
        size_t pos = output.find("MHz");
        if (pos != std::string::npos) {
            std::string speed_str = output.substr(0, pos);
            // Remove non-numeric characters
            speed_str.erase(std::remove_if(speed_str.begin(), speed_str.end(), [](char c) { return !std::isdigit(c); }), speed_str.end());
            if (!speed_str.empty()) {
                try {
                    memory_info_.memory_speed_mhz = std::stod(speed_str);
                } catch (...) {
                    // Ignore parsing errors
                }
            }
        }
    }
#endif

    // If we couldn't determine the memory speed, try to measure it
    if (memory_info_.memory_speed_mhz <= 0) {
        // Estimate memory speed based on bandwidth
        if (memory_info_.memory_bandwidth_gbps > 0) {
            // Rough estimate: bandwidth (GB/s) / 8 bytes per transfer ≈ frequency (MHz)
            memory_info_.memory_speed_mhz = memory_info_.memory_bandwidth_gbps * 1000 / 8;
        } else {
            // Measure memory bandwidth first
            measureMemoryBandwidth();
            if (memory_info_.memory_bandwidth_gbps > 0) {
                memory_info_.memory_speed_mhz = memory_info_.memory_bandwidth_gbps * 1000 / 8;
            }
        }
    }
    
    return true; // Always return true since this is not critical
}

bool MemoryDetector::detectNUMATopology() {
    // Default to no NUMA
    memory_info_.numa_available = false;
    memory_info_.numa_node_count = 1;
    memory_info_.numa_nodes.clear();
    
#if defined(_WIN32)
    // Windows
    // Check if NUMA is supported
    ULONG highest_node_number = 0;
    if (GetNumaHighestNodeNumber(&highest_node_number)) {
        memory_info_.numa_node_count = highest_node_number + 1;
        memory_info_.numa_available = (memory_info_.numa_node_count > 1);
        
        // Get information for each NUMA node
        for (ULONG node = 0; node <= highest_node_number; ++node) {
            NumaNodeInfo node_info;
            node_info.node_id = node;
            
            // Get memory for this node
            ULONGLONG node_memory = 0;
            if (GetNumaAvailableMemoryNode(node, &node_memory)) {
                node_info.memory_bytes = node_memory;
            }
            
            // Get processor mask for this node
            GROUP_AFFINITY group_affinity;
            ULONGLONG processor_mask = 0;
            if (GetNumaNodeProcessorMask(node, &processor_mask)) {
                // Convert processor mask to list of CPU cores
                for (int i = 0; i < 64; ++i) {
                    if ((processor_mask >> i) & 1) {
                        node_info.cpu_cores.push_back(i);
                    }
                }
            }
            
            memory_info_.numa_nodes.push_back(node_info);
        }
    }
    
#elif defined(__APPLE__)
    // macOS
    // macOS doesn't expose NUMA topology directly
    // For now, assume a single NUMA node
    NumaNodeInfo node_info;
    node_info.node_id = 0;
    node_info.memory_bytes = memory_info_.total_physical_memory_bytes;
    
    // Add all CPU cores to this node
    int cpu_count = std::thread::hardware_concurrency();
    for (int i = 0; i < cpu_count; ++i) {
        node_info.cpu_cores.push_back(i);
    }
    
    memory_info_.numa_nodes.push_back(node_info);
    
#else
    // Linux and other Unix-like systems
    // Check if NUMA is available
#ifdef NUMA
    if (numa_available() > 0) {
        memory_info_.numa_available = true;
        memory_info_.numa_node_count = numa_num_configured_nodes();
        
        // Get information for each NUMA node
        for (int node = 0; node < memory_info_.numa_node_count; ++node) {
            NumaNodeInfo node_info;
            node_info.node_id = node;
            
            // Get memory for this node
            long long node_memory = numa_node_size64(node, NULL);
            if (node_memory > 0) {
                node_info.memory_bytes = node_memory;
            }
            
            // Get CPU cores for this node
            struct bitmask* cpus = numa_allocate_cpumask();
            if (cpus) {
                if (numa_node_to_cpus(node, cpus) == 0) {
                    for (int i = 0; i < numa_num_configured_cpus(); ++i) {
                        if (numa_bitmask_isbitset(cpus, i)) {
                            node_info.cpu_cores.push_back(i);
                        }
                    }
                }
                numa_free_cpumask(cpus);
            }
            
            memory_info_.numa_nodes.push_back(node_info);
        }
    } else {
        // NUMA not available, create a single node
        NumaNodeInfo node_info;
        node_info.node_id = 0;
        node_info.memory_bytes = memory_info_.total_physical_memory_bytes;
        
        // Add all CPU cores to this node
        int cpu_count = std::thread::hardware_concurrency();
        for (int i = 0; i < cpu_count; ++i) {
            node_info.cpu_cores.push_back(i);
        }
        
        memory_info_.numa_nodes.push_back(node_info);
    }
#else
    // NUMA library not available, try to get information from sysfs
    std::ifstream numa_nodes("/sys/devices/system/node/online");
    if (numa_nodes.is_open()) {
        std::string line;
        if (std::getline(numa_nodes, line)) {
            // Parse range like "0-3" or just "0"
            size_t dash_pos = line.find('-');
            if (dash_pos != std::string::npos) {
                int start = std::stoi(line.substr(0, dash_pos));
                int end = std::stoi(line.substr(dash_pos + 1));
                memory_info_.numa_node_count = end - start + 1;
                memory_info_.numa_available = (memory_info_.numa_node_count > 1);
                
                // Get information for each NUMA node
                for (int node = start; node <= end; ++node) {
                    NumaNodeInfo node_info;
                    node_info.node_id = node;
                    
                    // Get memory for this node
                    std::string mem_path = "/sys/devices/system/node/node" + std::to_string(node) + "/meminfo";
                    std::ifstream meminfo(mem_path);
                    if (meminfo.is_open()) {
                        std::string mem_line;
                        while (std::getline(meminfo, mem_line)) {
                            if (mem_line.find("Node " + std::to_string(node) + " MemTotal:") != std::string::npos) {
                                // Extract memory size (in kB)
                                size_t pos = mem_line.find(':');
                                if (pos != std::string::npos) {
                                    std::string value = mem_line.substr(pos + 1);
                                    // Remove non-numeric characters
                                    value.erase(std::remove_if(value.begin(), value.end(), [](char c) { return !std::isdigit(c); }), value.end());
                                    try {
                                        node_info.memory_bytes = std::stoull(value) * 1024; // Convert kB to bytes
                                    } catch (...) {
                                        // Ignore parsing errors
                                    }
                                }
                                break;
                            }
                        }
                    }
                    
                    // Get CPU cores for this node
                    std::string cpu_path = "/sys/devices/system/node/node" + std::to_string(node) + "/cpulist";
                    std::ifstream cpulist(cpu_path);
                    if (cpulist.is_open()) {
                        std::string cpu_line;
                        if (std::getline(cpulist, cpu_line)) {
                            // Parse CPU list like "0-3,8-11" or just "0,1,2,3"
                            std::stringstream ss(cpu_line);
                            std::string range;
                            while (std::getline(ss, range, ',')) {
                                size_t dash_pos = range.find('-');
                                if (dash_pos != std::string::npos) {
                                    int start = std::stoi(range.substr(0, dash_pos));
                                    int end = std::stoi(range.substr(dash_pos + 1));
                                    for (int cpu = start; cpu <= end; ++cpu) {
                                        node_info.cpu_cores.push_back(cpu);
                                    }
                                } else {
                                    // Just one number
                                    node_info.cpu_cores.push_back(std::stoi(range));
                                }
                            }
                        }
                    }
                    
                    memory_info_.numa_nodes.push_back(node_info);
                }
            } else {
                // Just one node
                memory_info_.numa_node_count = 1;
                memory_info_.numa_available = false;
                
                NumaNodeInfo node_info;
                node_info.node_id = 0;
                node_info.memory_bytes = memory_info_.total_physical_memory_bytes;
                
                // Add all CPU cores to this node
                int cpu_count = std::thread::hardware_concurrency();
                for (int i = 0; i < cpu_count; ++i) {
                    node_info.cpu_cores.push_back(i);
                }
                
                memory_info_.numa_nodes.push_back(node_info);
            }
        }
    } else {
        // NUMA information not available, create a single node
        NumaNodeInfo node_info;
        node_info.node_id = 0;
        node_info.memory_bytes = memory_info_.total_physical_memory_bytes;
        
        // Add all CPU cores to this node
        int cpu_count = std::thread::hardware_concurrency();
        for (int i = 0; i < cpu_count; ++i) {
            node_info.cpu_cores.push_back(i);
        }
        
        memory_info_.numa_nodes.push_back(node_info);
    }
#endif
#endif

    return true; // Always return true since this is not critical
}

bool MemoryDetector::detectMemoryBandwidth() {
    // Measure memory bandwidth
    return measureMemoryBandwidth();
}

bool MemoryDetector::measureMemoryBandwidth() {
    // Measure memory bandwidth using a simple benchmark
    const size_t buffer_size = 1024 * 1024 * 100; // 100 MB
    const int iterations = 10;
    
    try {
        // Allocate memory
        std::vector<char> buffer(buffer_size);
        
        // Fill with random data
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        for (size_t i = 0; i < buffer_size; ++i) {
            buffer[i] = static_cast<char>(dis(gen));
        }
        
        // Measure read bandwidth
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for (int iter = 0; iter < iterations; ++iter) {
            volatile char sum = 0;
            for (size_t i = 0; i < buffer_size; ++i) {
                sum += buffer[i];
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
        
        // Calculate bandwidth in GB/s
        double seconds = duration / 1000000.0;
        double bytes_processed = buffer_size * iterations;
        double bandwidth_gbps = (bytes_processed / seconds) / (1024 * 1024 * 1024);
        
        memory_info_.memory_bandwidth_gbps = bandwidth_gbps;
        
        return true;
    } catch (const std::exception& e) {
        // Failed to measure bandwidth
        return false;
    }
}

bool MemoryDetector::executeCommand(const std::string& command, std::string& output) {
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

//=============================================================================
// StorageDetector Implementation
//=============================================================================

StorageDetector::StorageDetector() {
    // Initialize with default values
}

StorageDetector::~StorageDetector() {
    // Clean up resources if needed
}

bool StorageDetector::detect() {
    if (initialized_) {
        return true; // Already initialized
    }
    
    bool success = true;
    
    // Detect storage devices
    success &= detectStorageDevices();
    success &= detectStorageTypes();
    success &= detectStoragePerformance();
    
    initialized_ = success;
    return success;
}

const std::vector<StorageInfo>& StorageDetector::getStorageInfo() const {
    return storage_info_;
}

const StorageInfo* StorageDetector::getOptimalDatabaseStorage() const {
    if (!initialized_ || storage_info_.empty()) {
        return nullptr;
    }
    
    // Find the fastest storage device with enough space
    const StorageInfo* best_storage = nullptr;
    double best_score = 0.0;
    
    for (const auto& storage : storage_info_) {
        // Calculate a score based on performance metrics
        double read_score = storage.sequential_read_mbps * 0.4 + storage.random_read_iops * 0.1;
        double write_score = storage.sequential_write_mbps * 0.3 + storage.random_write_iops * 0.1;
        double latency_score = 1000.0 / (storage.access_time_ms + 1.0); // Inverse of access time
        
        double score = read_score + write_score + latency_score;
        
        // Check if this device has enough space (at least 10GB)
        const size_t min_space = 10ULL * 1024 * 1024 * 1024; // 10 GB
        if (storage.available_bytes >= min_space) {
            if (best_storage == nullptr || score > best_score) {
                best_storage = &storage;
                best_score = score;
            }
        }
    }
    
    return best_storage;
}

IOParameters StorageDetector::getOptimalIOParameters(const StorageInfo& storage) const {
    IOParameters params;
    
    // Set default values
    params.optimal_block_size = 4096; // 4 KB
    params.optimal_queue_depth = 32;
    params.optimal_thread_count = 4;
    params.use_direct_io = false;
    params.use_async_io = true;
    params.optimal_buffer_size = 64 * 1024 * 1024; // 64 MB
    params.optimal_file_size = 1024 * 1024 * 1024; // 1 GB
    
    // Adjust based on storage type
    switch (storage.type) {
        case StorageType::HDD:
            // HDDs benefit from larger block sizes and sequential access
            params.optimal_block_size = 1024 * 1024; // 1 MB
            params.optimal_queue_depth = 8;
            params.optimal_thread_count = 2;
            params.use_direct_io = false;
            params.use_async_io = true;
            params.optimal_buffer_size = 128 * 1024 * 1024; // 128 MB
            break;
            
        case StorageType::SSD:
            // SSDs benefit from parallel operations
            params.optimal_block_size = 64 * 1024; // 64 KB
            params.optimal_queue_depth = 32;
            params.optimal_thread_count = 8;
            params.use_direct_io = true;
            params.use_async_io = true;
            params.optimal_buffer_size = 64 * 1024 * 1024; // 64 MB
            break;
            
        case StorageType::NVME:
            // NVMe drives can handle high parallelism
            params.optimal_block_size = 32 * 1024; // 32 KB
            params.optimal_queue_depth = 64;
            params.optimal_thread_count = 16;
            params.use_direct_io = true;
            params.use_async_io = true;
            params.optimal_buffer_size = 32 * 1024 * 1024; // 32 MB
            break;
            
        default:
            // Use default values
            break;
    }
    
    // Adjust based on performance metrics
    if (storage.sequential_read_mbps > 1000) {
        // High-performance storage
        params.optimal_block_size *= 2;
        params.optimal_queue_depth *= 2;
    }
    
    if (storage.random_read_iops > 10000) {
        // High IOPS storage
        params.optimal_thread_count *= 2;
    }
    
    return params;
}

bool StorageDetector::measureStoragePerformance(StorageInfo& storage) {
    bool success = true;
    
    success &= measureSequentialReadSpeed(storage);
    success &= measureSequentialWriteSpeed(storage);
    success &= measureRandomReadSpeed(storage);
    success &= measureRandomWriteSpeed(storage);
    
    return success;
}

std::string StorageDetector::getSummary() const {
    if (!initialized_) {
        return "Storage detection not initialized";
    }
    
    std::stringstream ss;
    ss << "Storage Information:\n";
    ss << "  Number of Storage Devices: " << storage_info_.size() << "\n\n";
    
    for (size_t i = 0; i < storage_info_.size(); ++i) {
        const auto& storage = storage_info_[i];
        ss << "  Storage Device " << (i + 1) << ":\n";
        ss << "    Path: " << storage.device_path << "\n";
        ss << "    Mount Point: " << storage.mount_point << "\n";
        ss << "    Type: " << static_cast<int>(storage.type) << " (" 
           << (storage.type == StorageType::HDD ? "HDD" : 
               storage.type == StorageType::SSD ? "SSD" : 
               storage.type == StorageType::NVME ? "NVMe" : 
               storage.type == StorageType::RAID ? "RAID" : 
               storage.type == StorageType::SAN ? "SAN" : 
               storage.type == StorageType::NAS ? "NAS" : "Unknown") << ")\n";
        ss << "    Total Size: " << (storage.total_bytes / (1024 * 1024 * 1024)) << " GB\n";
        ss << "    Available Size: " << (storage.available_bytes / (1024 * 1024 * 1024)) << " GB\n";
        ss << "    Sequential Read: " << storage.sequential_read_mbps << " MB/s\n";
        ss << "    Sequential Write: " << storage.sequential_write_mbps << " MB/s\n";
        ss << "    Random Read: " << storage.random_read_iops << " IOPS\n";
        ss << "    Random Write: " << storage.random_write_iops << " IOPS\n";
        ss << "    Access Time: " << storage.access_time_ms << " ms\n";
        ss << "    Filesystem: " << storage.filesystem_type << "\n";
        ss << "    Rotational: " << (storage.is_rotational ? "Yes" : "No") << "\n";
        ss << "    Removable: " << (storage.is_removable ? "Yes" : "No") << "\n\n";
    }
    
    return ss.str();
}

bool StorageDetector::detectStorageDevices() {
    storage_info_.clear();
    
#if defined(_WIN32)
    // Windows
    // Get logical drives
    char drives[256];
    if (GetLogicalDriveStrings(sizeof(drives), drives)) {
        char* drive = drives;
        while (*drive) {
            StorageInfo info;
            info.device_path = drive;
            info.mount_point = drive;
            
            // Get drive type
            UINT drive_type = GetDriveType(drive);
            if (drive_type == DRIVE_FIXED) {
                // Get disk space information
                ULARGE_INTEGER total_bytes, free_bytes;
                if (GetDiskFreeSpaceEx(drive, NULL, &total_bytes, &free_bytes)) {
                    info.total_bytes = total_bytes.QuadPart;
                    info.available_bytes = free_bytes.QuadPart;
                }
                
                // Get filesystem type
                char fs_name[MAX_PATH];
                if (GetVolumeInformation(drive, NULL, 0, NULL, NULL, NULL, fs_name, sizeof(fs_name))) {
                    info.filesystem_type = fs_name;
                }
                
                // Try to determine if it's an SSD or HDD
                // This requires WMI queries, which is complex
                // For now, use a heuristic based on performance
                info.type = StorageType::UNKNOWN;
                
                storage_info_.push_back(info);
            }
            
            // Move to next drive
            drive += strlen(drive) + 1;
        }
    }
    
#elif defined(__APPLE__)
    // macOS
    // Get mounted filesystems
    struct statfs* mounts;
    int count = getmntinfo(&mounts, MNT_WAIT);
    
    for (int i = 0; i < count; ++i) {
        // Skip non-local filesystems
        if (strcmp(mounts[i].f_fstypename, "hfs") != 0 && 
            strcmp(mounts[i].f_fstypename, "apfs") != 0 && 
            strcmp(mounts[i].f_fstypename, "msdos") != 0 && 
            strcmp(mounts[i].f_fstypename, "ntfs") != 0 && 
            strcmp(mounts[i].f_fstypename, "exfat") != 0) {
            continue;
        }
        
        StorageInfo info;
        info.device_path = mounts[i].f_mntfromname;
        info.mount_point = mounts[i].f_mntonname;
        info.filesystem_type = mounts[i].f_fstypename;
        
        // Get disk space information
        info.total_bytes = mounts[i].f_blocks * mounts[i].f_bsize;
        info.available_bytes = mounts[i].f_bavail * mounts[i].f_bsize;
        
        // Try to determine if it's an SSD or HDD
        // Use diskutil to get information
        std::string output;
        std::string command = "diskutil info " + info.device_path + " | grep 'Solid State'";
        if (executeCommand(command, output)) {
            if (output.find("Yes") != std::string::npos) {
                info.type = StorageType::SSD;
                info.is_rotational = false;
            } else {
                info.type = StorageType::HDD;
                info.is_rotational = true;
            }
        } else {
            info.type = StorageType::UNKNOWN;
        }
        
        storage_info_.push_back(info);
    }
    
#else
    // Linux and other Unix-like systems
    // Get mounted filesystems
    std::ifstream mounts("/proc/mounts");
    if (mounts.is_open()) {
        std::string line;
        while (std::getline(mounts, line)) {
            std::istringstream iss(line);
            std::string device, mount_point, fs_type;
            if (iss >> device >> mount_point >> fs_type) {
                // Skip non-local filesystems
                if (fs_type == "proc" || fs_type == "sysfs" || fs_type == "devtmpfs" || 
                    fs_type == "tmpfs" || fs_type == "devpts" || fs_type == "cgroup" || 
                    fs_type == "debugfs" || fs_type == "securityfs" || fs_type == "pstore" || 
                    fs_type == "autofs" || fs_type == "mqueue" || fs_type == "hugetlbfs" || 
                    fs_type == "fusectl" || fs_type == "fuse.gvfsd-fuse" || fs_type == "binfmt_misc") {
                    continue;
                }
                
                StorageInfo info;
                info.device_path = device;
                info.mount_point = mount_point;
                info.filesystem_type = fs_type;
                
                // Get disk space information
                struct statvfs stat;
                if (statvfs(mount_point.c_str(), &stat) == 0) {
                    info.total_bytes = stat.f_blocks * stat.f_frsize;
                    info.available_bytes = stat.f_bavail * stat.f_frsize;
                }
                
                // Try to determine if it's an SSD or HDD
                // Check if the device is rotational
                std::string device_name = device;
                if (device_name.find("/dev/") == 0) {
                    device_name = device_name.substr(5);
                }
                
                // Remove partition number
                while (!device_name.empty() && std::isdigit(device_name.back())) {
                    device_name.pop_back();
                }
                
                // Remove trailing non-alphanumeric characters
                while (!device_name.empty() && !std::isalnum(device_name.back())) {
                    device_name.pop_back();
                }
                
                std::string rotational_path = "/sys/block/" + device_name + "/queue/rotational";
                std::ifstream rotational_file(rotational_path);
                if (rotational_file.is_open()) {
                    std::string value;
                    if (std::getline(rotational_file, value)) {
                        if (value == "0") {
                            info.type = StorageType::SSD;
                            info.is_rotational = false;
                        } else {
                            info.type = StorageType::HDD;
                            info.is_rotational = true;
                        }
                    }
                }
                
                // Check if it's an NVMe device
                if (device_name.find("nvme") == 0) {
                    info.type = StorageType::NVME;
                    info.is_rotational = false;
                }
                
                // Check if it's removable
                std::string removable_path = "/sys/block/" + device_name + "/removable";
                std::ifstream removable_file(removable_path);
                if (removable_file.is_open()) {
                    std::string value;
                    if (std::getline(removable_file, value)) {
                        info.is_removable = (value == "1");
                    }
                }
                
                storage_info_.push_back(info);
            }
        }
    }
#endif

    return !storage_info_.empty();
}

bool StorageDetector::detectStorageTypes() {
    // Most of the storage type detection is done in detectStorageDevices()
    // This function is mainly for additional type information
    
    // If we already have type information for all devices, return true
    bool all_types_known = true;
    for (const auto& storage : storage_info_) {
        if (storage.type == StorageType::UNKNOWN) {
            all_types_known = false;
            break;
        }
    }
    
    if (all_types_known) {
        return true;
    }
    
    // Try to determine storage types
    for (auto& storage : storage_info_) {
        if (storage.type != StorageType::UNKNOWN) {
            continue;
        }
        
        // Try to determine type based on performance
        if (storage.sequential_read_mbps > 0 || storage.random_read_iops > 0) {
            // If we have performance metrics, use them to guess the type
            if (storage.sequential_read_mbps > 1000 || storage.random_read_iops > 10000) {
                storage.type = StorageType::SSD;
                storage.is_rotational = false;
            } else {
                storage.type = StorageType::HDD;
                storage.is_rotational = true;
            }
        } else {
            // If we don't have performance metrics, try to measure them
            measureStoragePerformance(storage);
            
            // Now use the metrics to guess the type
            if (storage.sequential_read_mbps > 1000 || storage.random_read_iops > 10000) {
                storage.type = StorageType::SSD;
                storage.is_rotational = false;
            } else {
                storage.type = StorageType::HDD;
                storage.is_rotational = true;
            }
        }
    }
    
    return true;
}

bool StorageDetector::detectStoragePerformance() {
    // Measure performance for each storage device
    for (auto& storage : storage_info_) {
        measureStoragePerformance(storage);
    }
    
    return true;
}

bool StorageDetector::measureSequentialReadSpeed(StorageInfo& storage) {
    try {
        // Create a temporary file for testing
        std::string test_file = storage.mount_point + "/storage_test_read.tmp";
        const size_t file_size = 100 * 1024 * 1024; // 100 MB
        
        // Create the file with random data
        {
            std::ofstream file(test_file, std::ios::binary);
            if (!file.is_open()) {
                return false;
            }
            
            std::vector<char> buffer(1024 * 1024); // 1 MB buffer
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, 255);
            
            for (size_t i = 0; i < file_size; i += buffer.size()) {
                // Fill buffer with random data
                for (size_t j = 0; j < buffer.size(); ++j) {
                    buffer[j] = static_cast<char>(dis(gen));
                }
                
                // Write buffer to file
                file.write(buffer.data(), buffer.size());
            }
        }
        
        // Measure sequential read speed
        {
            std::ifstream file(test_file, std::ios::binary);
            if (!file.is_open()) {
                std::filesystem::remove(test_file);
                return false;
            }
            
            std::vector<char> buffer(1024 * 1024); // 1 MB buffer
            size_t total_bytes_read = 0;
            
            auto start_time = std::chrono::high_resolution_clock::now();
            
            while (file && total_bytes_read < file_size) {
                file.read(buffer.data(), buffer.size());
                size_t bytes_read = file.gcount();
                if (bytes_read == 0) {
                    break;
                }
                total_bytes_read += bytes_read;
            }
            
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
            
            // Calculate speed in MB/s
            double seconds = duration / 1000000.0;
            double megabytes = total_bytes_read / (1024.0 * 1024.0);
            storage.sequential_read_mbps = megabytes / seconds;
        }
        
        // Clean up
        std::filesystem::remove(test_file);
        
        return true;
    } catch (const std::exception& e) {
        // Failed to measure sequential read speed
        return false;
    }
}

bool StorageDetector::measureSequentialWriteSpeed(StorageInfo& storage) {
    try {
        // Create a temporary file for testing
        std::string test_file = storage.mount_point + "/storage_test_write.tmp";
        const size_t file_size = 100 * 1024 * 1024; // 100 MB
        
        // Measure sequential write speed
        {
            std::ofstream file(test_file, std::ios::binary);
            if (!file.is_open()) {
                return false;
            }
            
            std::vector<char> buffer(1024 * 1024); // 1 MB buffer
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, 255);
            
            // Fill buffer with random data
            for (size_t j = 0; j < buffer.size(); ++j) {
                buffer[j] = static_cast<char>(dis(gen));
            }
            
            auto start_time = std::chrono::high_resolution_clock::now();
            
            size_t total_bytes_written = 0;
            while (file && total_bytes_written < file_size) {
                file.write(buffer.data(), buffer.size());
                if (!file) {
                    break;
                }
                total_bytes_written += buffer.size();
            }
            
            file.flush();
            
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
            
            // Calculate speed in MB/s
            double seconds = duration / 1000000.0;
            double megabytes = total_bytes_written / (1024.0 * 1024.0);
            storage.sequential_write_mbps = megabytes / seconds;
        }
        
        // Clean up
        std::filesystem::remove(test_file);
        
        return true;
    } catch (const std::exception& e) {
        // Failed to measure sequential write speed
        return false;
    }
}

bool StorageDetector::measureRandomReadSpeed(StorageInfo& storage) {
    try {
        // Create a temporary file for testing
        std::string test_file = storage.mount_point + "/storage_test_random_read.tmp";
        const size_t file_size = 100 * 1024 * 1024; // 100 MB
        const size_t block_size = 4096; // 4 KB
        const int num_operations = 1000;
        
        // Create the file with random data
        {
            std::ofstream file(test_file, std::ios::binary);
            if (!file.is_open()) {
                return false;
            }
            
            std::vector<char> buffer(1024 * 1024); // 1 MB buffer
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, 255);
            
            for (size_t i = 0; i < file_size; i += buffer.size()) {
                // Fill buffer with random data
                for (size_t j = 0; j < buffer.size(); ++j) {
                    buffer[j] = static_cast<char>(dis(gen));
                }
                
                // Write buffer to file
                file.write(buffer.data(), buffer.size());
            }
        }
        
        // Measure random read speed
        {
            std::ifstream file(test_file, std::ios::binary);
            if (!file.is_open()) {
                std::filesystem::remove(test_file);
                return false;
            }
            
            std::vector<char> buffer(block_size);
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, static_cast<int>((file_size - block_size) / block_size));
            
            auto start_time = std::chrono::high_resolution_clock::now();
            
            for (int i = 0; i < num_operations; ++i) {
                // Generate random position
                size_t pos = dis(gen) * block_size;
                
                // Seek to position
                file.seekg(pos);
                
                // Read block
                file.read(buffer.data(), buffer.size());
                if (!file) {
                    break;
                }
            }
            
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
            
            // Calculate IOPS
            double seconds = duration / 1000000.0;
            storage.random_read_iops = num_operations / seconds;
            
            // Calculate access time
            storage.access_time_ms = (duration / 1000.0) / num_operations;
        }
        
        // Clean up
        std::filesystem::remove(test_file);
        
        return true;
    } catch (const std::exception& e) {
        // Failed to measure random read speed
        return false;
    }
}

bool StorageDetector::measureRandomWriteSpeed(StorageInfo& storage) {
    try {
        // Create a temporary file for testing
        std::string test_file = storage.mount_point + "/storage_test_random_write.tmp";
        const size_t file_size = 100 * 1024 * 1024; // 100 MB
        const size_t block_size = 4096; // 4 KB
        const int num_operations = 1000;
        
        // Create the file with zeros
        {
            std::ofstream file(test_file, std::ios::binary);
            if (!file.is_open()) {
                return false;
            }
            
            std::vector<char> buffer(1024 * 1024, 0); // 1 MB buffer of zeros
            
            for (size_t i = 0; i < file_size; i += buffer.size()) {
                // Write buffer to file
                file.write(buffer.data(), buffer.size());
            }
        }
        
        // Measure random write speed
        {
            std::fstream file(test_file, std::ios::binary | std::ios::in | std::ios::out);
            if (!file.is_open()) {
                std::filesystem::remove(test_file);
                return false;
            }
            
            std::vector<char> buffer(block_size);
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, static_cast<int>((file_size - block_size) / block_size));
            std::uniform_int_distribution<> data_dis(0, 255);
            
            auto start_time = std::chrono::high_resolution_clock::now();
            
            for (int i = 0; i < num_operations; ++i) {
                // Generate random position
                size_t pos = dis(gen) * block_size;
                
                // Fill buffer with random data
                for (size_t j = 0; j < buffer.size(); ++j) {
                    buffer[j] = static_cast<char>(data_dis(gen));
                }
                
                // Seek to position
                file.seekp(pos);
                
                // Write block
                file.write(buffer.data(), buffer.size());
                if (!file) {
                    break;
                }
            }
            
            file.flush();
            
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
            
            // Calculate IOPS
            double seconds = duration / 1000000.0;
            storage.random_write_iops = num_operations / seconds;
        }
        
        // Clean up
        std::filesystem::remove(test_file);
        
        return true;
    } catch (const std::exception& e) {
        // Failed to measure random write speed
        return false;
    }
}

bool StorageDetector::executeCommand(const std::string& command, std::string& output) {
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

} // namespace system
} // namespace mfp
