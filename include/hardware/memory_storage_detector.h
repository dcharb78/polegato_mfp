#pragma once

#include <string>
#include <vector>

namespace mfp {

// Memory information structure
struct MemoryInfo {
    uint64_t total_physical_bytes = 0;
    uint64_t available_physical_bytes = 0;
    uint64_t total_virtual_bytes = 0;
    uint64_t available_virtual_bytes = 0;
    uint64_t total_swap_bytes = 0;
    uint64_t available_swap_bytes = 0;
    
    // Memory type information
    std::string memory_type;  // DDR4, DDR5, LPDDR4, etc.
    double memory_speed_mhz = 0.0;
    int memory_channels = 0;
    
    // NUMA information
    int numa_nodes = 0;
    std::vector<int> numa_node_sizes;  // Size in MB for each NUMA node
    
    // Get a human-readable summary
    std::string getSummary() const;
};

// Storage device types
enum class StorageType {
    UNKNOWN,
    HDD,
    SSD,
    NVME,
    RAID,
    NETWORK,
    RAM_DISK
};

// Storage device information
struct StorageDeviceInfo {
    std::string device_path;
    std::string mount_point;
    std::string filesystem_type;
    StorageType type = StorageType::UNKNOWN;
    uint64_t total_bytes = 0;
    uint64_t available_bytes = 0;
    
    // Performance metrics
    double sequential_read_mbps = 0.0;
    double sequential_write_mbps = 0.0;
    double random_read_iops = 0.0;
    double random_write_iops = 0.0;
    
    // Get a human-readable summary
    std::string getSummary() const;
};

// Complete memory and storage information
class MemoryStorageDetector {
public:
    MemoryStorageDetector();
    ~MemoryStorageDetector();
    
    // Detect memory and storage capabilities
    void detect();
    
    // Getters
    const MemoryInfo& getMemoryInfo() const { return m_memoryInfo; }
    const std::vector<StorageDeviceInfo>& getStorageDevices() const { return m_storageDevices; }
    
    // Find the best storage device for database operations
    StorageDeviceInfo findBestDatabaseStorage() const;
    
    // Calculate optimal memory allocation for a specific workload
    uint64_t getOptimalMemoryAllocation(bool large_dataset = false) const;
    
    // Get a human-readable summary
    std::string getSummary() const;
    
private:
    MemoryInfo m_memoryInfo;
    std::vector<StorageDeviceInfo> m_storageDevices;
    
    // Platform-specific detection methods
    void detectMemoryOnLinux();
    void detectMemoryOnMacOS();
    void detectMemoryOnWindows();
    
    void detectStorageOnLinux();
    void detectStorageOnMacOS();
    void detectStorageOnWindows();
    
    // Helper methods
    StorageType determineStorageType(const std::string& device_path);
    void measureStoragePerformance(StorageDeviceInfo& device);
};

} // namespace mfp
