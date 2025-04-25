#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace mfp {
namespace system {

// Memory type enumeration
enum class MemoryType {
    DDR3,
    DDR4,
    DDR5,
    LPDDR4,
    LPDDR5,
    HBM,
    HBM2,
    UNKNOWN
};

// NUMA node information structure
struct NumaNodeInfo {
    int node_id = 0;
    size_t memory_bytes = 0;
    std::vector<int> cpu_cores;
};

// Memory information structure
struct MemoryInfo {
    size_t total_physical_memory_bytes = 0;
    size_t available_physical_memory_bytes = 0;
    size_t total_virtual_memory_bytes = 0;
    size_t available_virtual_memory_bytes = 0;
    size_t page_size_bytes = 0;
    int memory_channels = 0;
    double memory_speed_mhz = 0.0;
    double memory_bandwidth_gbps = 0.0;
    MemoryType memory_type = MemoryType::UNKNOWN;
    bool numa_available = false;
    int numa_node_count = 0;
    std::vector<NumaNodeInfo> numa_nodes;
    std::map<std::string, std::string> additional_info;
};

// Storage type enumeration
enum class StorageType {
    HDD,
    SSD,
    NVME,
    RAID,
    SAN,
    NAS,
    UNKNOWN
};

// I/O parameters structure
struct IOParameters {
    size_t optimal_block_size = 0;
    int optimal_queue_depth = 0;
    int optimal_thread_count = 0;
    bool use_direct_io = false;
    bool use_async_io = false;
    size_t optimal_buffer_size = 0;
    size_t optimal_file_size = 0;
};

// Storage information structure
struct StorageInfo {
    std::string device_path;
    std::string mount_point;
    StorageType type = StorageType::UNKNOWN;
    size_t total_bytes = 0;
    size_t available_bytes = 0;
    double sequential_read_mbps = 0.0;
    double sequential_write_mbps = 0.0;
    double random_read_iops = 0.0;
    double random_write_iops = 0.0;
    double access_time_ms = 0.0;
    bool is_removable = false;
    bool is_rotational = false;
    std::string filesystem_type;
    std::map<std::string, std::string> additional_info;
};

// Memory detector class
class MemoryDetector {
public:
    // Constructor and destructor
    MemoryDetector();
    ~MemoryDetector();
    
    // Detect memory capabilities
    bool detect();
    
    // Get memory information
    const MemoryInfo& getMemoryInfo() const;
    
    // Get optimal memory allocation size for different workloads
    size_t getOptimalAllocationSize(WorkloadType workload) const;
    
    // Check if NUMA is available
    bool hasNUMA() const;
    
    // Get number of NUMA nodes
    int getNumaNodeCount() const;
    
    // Get memory bandwidth
    double getMemoryBandwidth() const;
    
    // Get memory summary
    std::string getSummary() const;
    
private:
    // Memory information
    MemoryInfo memory_info_;
    
    // Detection methods
    bool detectTotalMemory();
    bool detectAvailableMemory();
    bool detectMemorySpeed();
    bool detectNUMATopology();
    bool detectMemoryBandwidth();
    bool detectPageSize();
    bool detectMemoryType();
    
    // Platform-specific detection
    bool detectMemoryInfoLinux();
    bool detectMemoryInfoWindows();
    bool detectMemoryInfoMacOS();
    
    // Helper methods
    bool measureMemoryBandwidth();
    bool executeCommand(const std::string& command, std::string& output);
    
    // State
    bool initialized_ = false;
};

// Storage detector class
class StorageDetector {
public:
    // Constructor and destructor
    StorageDetector();
    ~StorageDetector();
    
    // Detect storage capabilities
    bool detect();
    
    // Get storage information
    const std::vector<StorageInfo>& getStorageInfo() const;
    
    // Get optimal storage device for database
    const StorageInfo* getOptimalDatabaseStorage() const;
    
    // Get optimal I/O parameters for a specific storage device
    IOParameters getOptimalIOParameters(const StorageInfo& storage) const;
    
    // Measure storage performance
    bool measureStoragePerformance(StorageInfo& storage);
    
    // Get storage summary
    std::string getSummary() const;
    
private:
    // Storage information
    std::vector<StorageInfo> storage_info_;
    
    // Detection methods
    bool detectStorageDevices();
    bool detectStorageTypes();
    bool detectStoragePerformance();
    
    // Platform-specific detection
    bool detectStorageInfoLinux();
    bool detectStorageInfoWindows();
    bool detectStorageInfoMacOS();
    
    // Helper methods
    bool measureSequentialReadSpeed(StorageInfo& storage);
    bool measureSequentialWriteSpeed(StorageInfo& storage);
    bool measureRandomReadSpeed(StorageInfo& storage);
    bool measureRandomWriteSpeed(StorageInfo& storage);
    bool executeCommand(const std::string& command, std::string& output);
    
    // State
    bool initialized_ = false;
};

} // namespace system
} // namespace mfp
