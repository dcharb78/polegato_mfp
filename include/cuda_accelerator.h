#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "gpu_detector.h"

namespace mfp {
namespace cuda {

// Forward declarations
class CUDAContext;
class CUDAStream;
class CUDAMemory;
class CUDAKernel;

// CUDA error handling
enum class CUDAStatus {
    SUCCESS,
    ERROR_INITIALIZATION,
    ERROR_MEMORY_ALLOCATION,
    ERROR_MEMORY_COPY,
    ERROR_KERNEL_LAUNCH,
    ERROR_SYNCHRONIZATION,
    ERROR_INVALID_DEVICE,
    ERROR_UNSUPPORTED_DEVICE,
    ERROR_INSUFFICIENT_MEMORY,
    ERROR_NOT_IMPLEMENTED,
    ERROR_UNKNOWN
};

// CUDA memory type
enum class CUDAMemoryType {
    HOST,
    DEVICE,
    UNIFIED,
    PINNED
};

// CUDA precision type
enum class CUDAPrecision {
    FP32,
    FP64,
    INT32,
    INT64
};

// CUDA context class
class CUDAContext {
public:
    // Constructor and destructor
    CUDAContext();
    ~CUDAContext();
    
    // Initialize CUDA context
    CUDAStatus initialize(int device_id = -1);
    
    // Get device properties
    const system::GPUInfo* getDeviceInfo() const;
    
    // Get CUDA status
    CUDAStatus getStatus() const;
    
    // Get error message
    std::string getErrorMessage() const;
    
    // Check if CUDA is available
    bool isAvailable() const;
    
    // Get device count
    int getDeviceCount() const;
    
    // Set device
    CUDAStatus setDevice(int device_id);
    
    // Get current device
    int getCurrentDevice() const;
    
    // Synchronize device
    CUDAStatus synchronize();
    
    // Create stream
    std::shared_ptr<CUDAStream> createStream();
    
    // Create memory
    std::shared_ptr<CUDAMemory> allocateMemory(size_t size_bytes, CUDAMemoryType memory_type = CUDAMemoryType::DEVICE);
    
    // Create kernel
    std::shared_ptr<CUDAKernel> createKernel(const std::string& kernel_name, const std::string& kernel_source);
    
    // Load pre-compiled kernel
    std::shared_ptr<CUDAKernel> loadKernel(const std::string& kernel_name, const std::string& ptx_file);
    
private:
    // CUDA context state
    CUDAStatus status_;
    std::string error_message_;
    int device_count_;
    int current_device_;
    system::GPUInfo device_info_;
    void* cuda_context_; // Opaque pointer to CUDA context
    
    // CUDA runtime API wrappers
    CUDAStatus getDeviceCount(int& count);
    CUDAStatus getDeviceProperties(int device_id, system::GPUInfo& info);
};

// CUDA stream class
class CUDAStream {
public:
    // Constructor and destructor
    CUDAStream(CUDAContext* context);
    ~CUDAStream();
    
    // Synchronize stream
    CUDAStatus synchronize();
    
    // Get CUDA status
    CUDAStatus getStatus() const;
    
    // Get error message
    std::string getErrorMessage() const;
    
    // Get native stream handle
    void* getNativeHandle() const;
    
private:
    // CUDA stream state
    CUDAContext* context_;
    CUDAStatus status_;
    std::string error_message_;
    void* stream_; // Opaque pointer to CUDA stream
    
    // Friend classes
    friend class CUDAKernel;
    friend class CUDAMemory;
};

// CUDA memory class
class CUDAMemory {
public:
    // Constructor and destructor
    CUDAMemory(CUDAContext* context, size_t size_bytes, CUDAMemoryType memory_type);
    ~CUDAMemory();
    
    // Copy data from host to device
    CUDAStatus copyFromHost(const void* host_ptr, size_t size_bytes = 0, size_t offset = 0);
    
    // Copy data from device to host
    CUDAStatus copyToHost(void* host_ptr, size_t size_bytes = 0, size_t offset = 0);
    
    // Copy data from device to device
    CUDAStatus copyFromDevice(const CUDAMemory& source, size_t size_bytes = 0, size_t dest_offset = 0, size_t source_offset = 0);
    
    // Asynchronous copy from host to device
    CUDAStatus copyFromHostAsync(const void* host_ptr, CUDAStream& stream, size_t size_bytes = 0, size_t offset = 0);
    
    // Asynchronous copy from device to host
    CUDAStatus copyToHostAsync(void* host_ptr, CUDAStream& stream, size_t size_bytes = 0, size_t offset = 0);
    
    // Asynchronous copy from device to device
    CUDAStatus copyFromDeviceAsync(const CUDAMemory& source, CUDAStream& stream, size_t size_bytes = 0, size_t dest_offset = 0, size_t source_offset = 0);
    
    // Set memory
    CUDAStatus memset(int value, size_t size_bytes = 0, size_t offset = 0);
    
    // Asynchronous memset
    CUDAStatus memsetAsync(int value, CUDAStream& stream, size_t size_bytes = 0, size_t offset = 0);
    
    // Get pointer
    void* getPtr() const;
    
    // Get size
    size_t getSize() const;
    
    // Get memory type
    CUDAMemoryType getMemoryType() const;
    
    // Get CUDA status
    CUDAStatus getStatus() const;
    
    // Get error message
    std::string getErrorMessage() const;
    
private:
    // CUDA memory state
    CUDAContext* context_;
    CUDAStatus status_;
    std::string error_message_;
    void* ptr_;
    size_t size_bytes_;
    CUDAMemoryType memory_type_;
    
    // Friend classes
    friend class CUDAKernel;
};

// CUDA kernel class
class CUDAKernel {
public:
    // Constructor and destructor
    CUDAKernel(CUDAContext* context, const std::string& kernel_name);
    ~CUDAKernel();
    
    // Compile kernel from source
    CUDAStatus compile(const std::string& kernel_source);
    
    // Load pre-compiled kernel
    CUDAStatus load(const std::string& ptx_file);
    
    // Set kernel arguments
    template<typename... Args>
    CUDAStatus setArgs(Args&&... args);
    
    // Launch kernel
    CUDAStatus launch(dim3 grid_dim, dim3 block_dim, size_t shared_memory_bytes = 0);
    
    // Launch kernel with stream
    CUDAStatus launchAsync(dim3 grid_dim, dim3 block_dim, CUDAStream& stream, size_t shared_memory_bytes = 0);
    
    // Get CUDA status
    CUDAStatus getStatus() const;
    
    // Get error message
    std::string getErrorMessage() const;
    
private:
    // CUDA kernel state
    CUDAContext* context_;
    CUDAStatus status_;
    std::string error_message_;
    std::string kernel_name_;
    void* module_;
    void* function_;
    std::vector<void*> args_;
    
    // Helper methods
    template<typename T>
    void addArg(T arg);
    
    template<typename T, typename... Args>
    void addArgs(T first, Args&&... rest);
    
    void addArgs();
};

// MFP CUDA implementation class
class MFPCUDA {
public:
    // Constructor and destructor
    MFPCUDA();
    ~MFPCUDA();
    
    // Initialize CUDA
    bool initialize(int device_id = -1);
    
    // Check if CUDA is available
    bool isAvailable() const;
    
    // Get CUDA context
    CUDAContext* getContext() const;
    
    // Get device information
    const system::GPUInfo* getDeviceInfo() const;
    
    // Method 1: Expanded q Factorization on GPU
    bool runMethod1(const std::string& number, std::vector<std::string>& factors);
    
    // Method 2: Ultrafast with Structural Filter on GPU
    bool runMethod2(const std::string& number, std::vector<std::string>& factors);
    
    // Method 3: Parallelized with Dynamic Blocks on GPU
    bool runMethod3(const std::string& number, std::vector<std::string>& factors);
    
    // Check if number is prime using GPU
    bool isPrime(const std::string& number);
    
    // Find next prime using GPU
    std::string findNextPrime(const std::string& number);
    
    // Find prime factors using GPU
    bool findPrimeFactors(const std::string& number, std::vector<std::string>& factors);
    
    // Set performance logging
    void setPerformanceLogging(bool enable);
    
    // Get performance metrics
    std::string getPerformanceMetrics() const;
    
private:
    // CUDA state
    std::unique_ptr<CUDAContext> context_;
    bool performance_logging_enabled_;
    
    // Performance metrics
    struct PerformanceMetrics {
        double kernel_execution_time_ms;
        double memory_transfer_time_ms;
        double total_time_ms;
        size_t memory_used_bytes;
        int blocks_used;
        int threads_per_block;
        std::string method_name;
    };
    
    std::vector<PerformanceMetrics> performance_metrics_;
    
    // Helper methods
    bool initializeKernels();
    void logPerformance(const PerformanceMetrics& metrics);
    
    // CUDA kernels
    std::shared_ptr<CUDAKernel> method1_kernel_;
    std::shared_ptr<CUDAKernel> method2_kernel_;
    std::shared_ptr<CUDAKernel> method3_kernel_;
    std::shared_ptr<CUDAKernel> is_prime_kernel_;
    std::shared_ptr<CUDAKernel> find_next_prime_kernel_;
    
    // Implementation methods
    bool implementMethod1(const std::string& number, std::vector<std::string>& factors);
    bool implementMethod2(const std::string& number, std::vector<std::string>& factors);
    bool implementMethod3(const std::string& number, std::vector<std::string>& factors);
};

} // namespace cuda
} // namespace mfp
