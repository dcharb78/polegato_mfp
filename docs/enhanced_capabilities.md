# Enhanced MFP Implementation Documentation

## Overview

This document provides comprehensive documentation for the enhanced Modular Factorization Pattern (MFP) implementation. The implementation includes automatic hardware detection, GPU acceleration, and dynamic resource allocation to optimize performance across different hardware configurations.

## Table of Contents

1. [System Architecture](#system-architecture)
2. [Hardware Detection Framework](#hardware-detection-framework)
3. [GPU Acceleration](#gpu-acceleration)
4. [Dynamic Resource Allocation](#dynamic-resource-allocation)
5. [Configuration System](#configuration-system)
6. [Performance Metrics](#performance-metrics)
7. [Usage Guide](#usage-guide)
8. [API Reference](#api-reference)
9. [Testing and Validation](#testing-and-validation)
10. [Troubleshooting](#troubleshooting)

## System Architecture

The enhanced MFP implementation follows a layered architecture:

```
┌─────────────────────────────────────────────────────────────┐
│                      MFP Application                         │
├─────────────────────────────────────────────────────────────┤
│                     Configuration Manager                    │
├─────────────────────────────────────────────────────────────┤
│                      Resource Manager                        │
├───────────────┬───────────────────────────┬─────────────────┤
│ MFP Methods   │   Hardware Detection      │ GPU Acceleration │
│ - Method 1    │   - CPU Detector          │ - CUDA           │
│ - Method 2    │   - Memory Detector       │ - Metal          │
│ - Method 3    │   - Storage Detector      │                  │
│               │   - GPU Detector          │                  │
└───────────────┴───────────────────────────┴─────────────────┘
```

The architecture is designed to provide:

1. **Modularity**: Each component has a well-defined interface and can be developed and tested independently.
2. **Extensibility**: New MFP methods, hardware detection components, or acceleration technologies can be added without modifying existing code.
3. **Automatic Optimization**: The system automatically detects hardware capabilities and selects the optimal execution strategy.
4. **Fallback Mechanisms**: If a preferred execution strategy is unavailable, the system gracefully falls back to alternative strategies.

## Hardware Detection Framework

The hardware detection framework automatically identifies the capabilities of the host system, including:

### CPU Detection

The CPU detection component identifies:

- CPU architecture (x86, x86_64, ARM, ARM64)
- Number of physical and logical cores
- Hyperthreading status
- CPU features (AVX, AVX2, AVX-512, SSE4, etc.)
- Cache hierarchy (L1, L2, L3 cache sizes)
- CPU frequency information

```cpp
// Example of accessing CPU information
ResourceManager& resource_manager = getResourceManager();
const CPUInfo& cpu_info = resource_manager.getCPUInfo();
int physical_cores = cpu_info.physical_cores;
int logical_cores = cpu_info.logical_cores;
bool has_avx2 = cpu_info.has_avx2;
```

### Memory Detection

The memory detection component identifies:

- Total physical memory
- Available physical memory
- Total virtual memory
- Available virtual memory
- Memory type and speed (when available)
- NUMA topology (when applicable)

```cpp
// Example of accessing memory information
ResourceManager& resource_manager = getResourceManager();
const MemoryInfo& memory_info = resource_manager.getMemoryInfo();
size_t total_memory = memory_info.total_physical_memory;
size_t available_memory = memory_info.available_physical_memory;
```

### Storage Detection

The storage detection component identifies:

- Primary storage type (HDD, SSD, NVMe)
- Primary storage capacity
- Primary storage available space
- Storage performance characteristics (when available)

```cpp
// Example of accessing storage information
ResourceManager& resource_manager = getResourceManager();
const StorageInfo& storage_info = resource_manager.getStorageInfo();
std::string storage_type = storage_info.primary_storage_type;
size_t storage_capacity = storage_info.primary_storage_capacity;
```

### GPU Detection

The GPU detection component identifies:

- Available GPUs and their properties
- GPU vendor (NVIDIA, AMD, Intel, Apple)
- GPU memory capacity
- Supported APIs (CUDA, OpenCL, Metal, DirectX, Vulkan)
- Compute capabilities

```cpp
// Example of accessing GPU information
ResourceManager& resource_manager = getResourceManager();
const std::vector<GPUInfo>& gpus = resource_manager.getGPUs();
bool has_cuda = resource_manager.isCUDAAvailable();
bool has_metal = resource_manager.isMetalAvailable();
```

## GPU Acceleration

The GPU acceleration components enable the MFP implementation to leverage GPU hardware for accelerated computation:

### CUDA Acceleration

The CUDA acceleration component provides:

- Support for NVIDIA GPUs using CUDA
- Memory management for large numbers
- Kernel management for MFP operations
- Stream management for asynchronous execution
- Performance monitoring

```cpp
// Example of using CUDA acceleration
ResourceManager& resource_manager = getResourceManager();
resource_manager.setExecutionStrategy(ExecutionStrategy::CUDA_GPU);
auto mfp = resource_manager.createMFP(1);  // Create Method 1 with CUDA acceleration
```

### Metal Acceleration

The Metal acceleration component provides:

- Support for Apple GPUs using Metal
- Memory management for large numbers
- Shader management for MFP operations
- Command management for execution
- Performance monitoring

```cpp
// Example of using Metal acceleration
ResourceManager& resource_manager = getResourceManager();
resource_manager.setExecutionStrategy(ExecutionStrategy::METAL_GPU);
auto mfp = resource_manager.createMFP(1);  // Create Method 1 with Metal acceleration
```

## Dynamic Resource Allocation

The dynamic resource allocation system automatically selects the optimal execution strategy and resource configuration based on the detected hardware:

### Resource Manager

The resource manager provides:

- Hardware information access
- Execution strategy selection
- Allocation mode configuration
- Optimal thread count calculation
- Optimal memory limit calculation
- MFP implementation creation

```cpp
// Example of using the resource manager
ResourceManager& resource_manager = getResourceManager();
resource_manager.initialize();
resource_manager.setExecutionStrategy(ExecutionStrategy::AUTO);
resource_manager.setAllocationMode(AllocationMode::BALANCED);
auto mfp = resource_manager.createMFP(1);  // Create Method 1 with automatic strategy
```

### Execution Strategies

The system supports the following execution strategies:

- **AUTO**: Automatically select the best strategy based on available hardware
- **CPU_ONLY**: Use only CPU for computation
- **CUDA_GPU**: Use NVIDIA GPU with CUDA for computation
- **METAL_GPU**: Use Apple GPU with Metal for computation
- **HYBRID**: Use both CPU and GPU for computation (experimental)

### Allocation Modes

The system supports the following allocation modes:

- **AUTO**: Automatically select the best mode based on available hardware
- **PERFORMANCE**: Optimize for maximum performance
- **MEMORY**: Optimize for minimum memory usage
- **BALANCED**: Balance performance and memory usage

## Configuration System

The configuration system provides profile-based configuration management:

### Configuration Manager

The configuration manager provides:

- Profile management
- Parameter configuration
- Configuration persistence
- Hardware-specific profiles

```cpp
// Example of using the configuration manager
ConfigurationManager& config_manager = getConfigurationManager();
config_manager.initialize(getResourceManager());
const ConfigProfile& profile = config_manager.getCurrentProfile();
```

### Hardware Profiles

The system includes predefined profiles for different hardware classes:

- **LOW_END**: Optimized for low-end hardware (dual-core CPUs, limited RAM)
- **MID_RANGE**: Optimized for mid-range hardware (quad-core CPUs, moderate RAM)
- **HIGH_END**: Optimized for high-end hardware (8+ core CPUs, gaming GPUs)
- **SERVER**: Optimized for server hardware (16+ core CPUs, large RAM)
- **WORKSTATION**: Optimized for workstation hardware (high-end CPUs, professional GPUs)
- **CUSTOM**: User-defined custom configuration

## Performance Metrics

The system includes comprehensive performance monitoring capabilities:

### Metrics Collection

The performance metrics component collects:

- Execution time for operations
- Memory usage
- Thread utilization
- Operation counts
- I/O statistics

```cpp
// Example of using performance metrics
auto mfp = getResourceManager().createMFP(1);
mfp->enablePerformanceMetrics(true);
bool is_prime;
mfp->isPrime(104729, is_prime);
const PerformanceMetrics& metrics = mfp->getPerformanceMetrics();
double execution_time = metrics.total_execution_time_ms;
```

### Performance Reporting

The system can generate performance reports with:

- Execution time statistics
- Memory usage statistics
- Thread utilization statistics
- Operation counts
- Comparative analysis between methods

## Usage Guide

### Basic Usage

```cpp
#include "mfp_system.h"
#include <gmp.h>

int main() {
    // Initialize the system
    mfp::getResourceManager().initialize();
    mfp::getConfigurationManager().initialize(mfp::getResourceManager());
    
    // Create MFP implementation (automatically selects best method and strategy)
    auto mfp = mfp::getResourceManager().createMFP(1);
    
    // Enable performance metrics
    mfp->enablePerformanceMetrics(true);
    
    // Check if a number is prime
    mpz_t number;
    mpz_init_set_str(number, "104729", 10);
    
    bool is_prime;
    mfp->isPrime(number, is_prime);
    
    std::cout << "Is prime: " << (is_prime ? "Yes" : "No") << std::endl;
    
    // Get performance metrics
    const mfp::PerformanceMetrics& metrics = mfp->getPerformanceMetrics();
    std::cout << "Execution time: " << metrics.total_execution_time_ms << " ms" << std::endl;
    
    // Clean up
    mpz_clear(number);
    
    return 0;
}
```

### Command-Line Interface

The MFP application provides a command-line interface:

```
Usage: mfp_app <command> [options]

Commands:
  isprime <number>       Check if a number is prime
  factorize <number>     Factorize a number into its prime factors
  nextprime <number>     Find the next prime after a number
  benchmark [size]       Run a benchmark (default size: 1000 bits)
  sysinfo               Display system information

Options:
  --method <1|2|3>       Select MFP method (default: auto)
  --strategy <auto|cpu|cuda|metal|hybrid>
                         Select execution strategy (default: auto)
  --mode <auto|performance|memory|balanced>
                         Select allocation mode (default: auto)
  --profile <name>       Select configuration profile
  --metrics <on|off>     Enable/disable performance metrics (default: on)
  --help                 Display this help message
```

Examples:

```bash
# Check if a number is prime
./mfp_app isprime 104729

# Factorize a number
./mfp_app factorize 123456789

# Find the next prime after a number
./mfp_app nextprime 104729

# Run a benchmark with 2000-bit numbers
./mfp_app benchmark 2000

# Display system information
./mfp_app sysinfo

# Use Method 3 with CUDA acceleration
./mfp_app isprime 104729 --method 3 --strategy cuda
```

## API Reference

### MFP Base Class

```cpp
class MFPBase {
public:
    // Core MFP operations
    virtual bool isPrime(const mpz_t number, bool& result) = 0;
    virtual bool factorize(const mpz_t number, std::vector<mpz_t>& factors) = 0;
    virtual bool nextPrime(const mpz_t number, mpz_t next_prime) = 0;
    
    // Performance metrics
    virtual void enablePerformanceMetrics(bool enable) = 0;
    virtual const PerformanceMetrics& getPerformanceMetrics() const = 0;
    
    // Virtual destructor
    virtual ~MFPBase() {}
};
```

### Resource Manager

```cpp
class ResourceManager {
public:
    // Initialization
    bool initialize();
    
    // Hardware information
    const CPUInfo& getCPUInfo() const;
    const MemoryInfo& getMemoryInfo() const;
    const StorageInfo& getStorageInfo() const;
    const std::vector<GPUInfo>& getGPUs() const;
    GPUInfo getBestGPU() const;
    
    // Execution strategy
    void setExecutionStrategy(ExecutionStrategy strategy);
    ExecutionStrategy getExecutionStrategy() const;
    
    // Allocation mode
    void setAllocationMode(AllocationMode mode);
    AllocationMode getAllocationMode() const;
    
    // Resource optimization
    int getOptimalThreadCount() const;
    size_t getOptimalBlockSize() const;
    size_t getOptimalMemoryLimit() const;
    
    // GPU acceleration
    bool isCUDAAvailable() const;
    bool isMetalAvailable() const;
    
    // MFP creation
    std::unique_ptr<MFPBase> createMFP(int method_number);
    
    // System information
    std::string getSystemSummary() const;
};
```

### Configuration Manager

```cpp
class ConfigurationManager {
public:
    // Initialization
    bool initialize(ResourceManager& resource_manager);
    
    // Profile management
    const ConfigProfile& getCurrentProfile() const;
    void setCurrentProfile(const ConfigProfile& profile);
    const std::map<std::string, ConfigProfile>& getProfiles() const;
    void addProfile(const ConfigProfile& profile);
    void removeProfile(const std::string& name);
    
    // Configuration persistence
    bool saveToFile(const std::string& filename) const;
    bool loadFromFile(const std::string& filename);
    
    // Configuration information
    std::string getConfigurationSummary() const;
};
```

## Testing and Validation

The implementation includes comprehensive testing to ensure correctness and performance:

### Unit Tests

Unit tests verify the correctness of individual components:

- Hardware detection tests
- Resource allocation tests
- MFP operation tests
- Configuration management tests

### Integration Tests

Integration tests verify the interaction between components:

- Hardware detection with resource allocation
- Resource allocation with MFP operations
- Configuration management with resource allocation

### Performance Tests

Performance tests measure the efficiency of the implementation:

- Execution time for different MFP methods
- Execution time for different execution strategies
- Memory usage for different allocation modes
- Scaling with number size

### Validation Tests

Validation tests confirm that the implementation meets requirements:

- Correctness of prime number detection
- Correctness of factorization
- Correctness of next prime calculation
- Automatic hardware detection
- Dynamic resource allocation

## Troubleshooting

### Common Issues

#### GPU Acceleration Not Available

If GPU acceleration is not available:

1. Check if a compatible GPU is installed
2. Verify that the required drivers are installed
3. Check if the required libraries (CUDA, Metal) are installed
4. Try updating GPU drivers to the latest version

#### Performance Issues

If performance is lower than expected:

1. Check if the correct execution strategy is selected
2. Verify that the optimal thread count is being used
3. Check if the system has sufficient memory
4. Try different allocation modes
5. Consider using a different MFP method

#### Compilation Issues

If compilation fails:

1. Verify that all dependencies are installed
2. Check if the compiler supports C++17
3. Ensure that the GMP library is properly installed
4. Check for platform-specific issues

### Getting Help

If you encounter issues not covered in this documentation:

1. Check the GitHub repository for known issues
2. Run the system information command to gather diagnostic information:
   ```
   ./mfp_app sysinfo
   ```
3. Submit an issue on GitHub with the system information and a description of the problem
