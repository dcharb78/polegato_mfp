#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "gpu_detector.h"

namespace mfp {
namespace metal {

// Forward declarations
class MetalContext;
class MetalCommandQueue;
class MetalBuffer;
class MetalComputePipeline;

// Metal error handling
enum class MetalStatus {
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
    ERROR_SHADER_COMPILATION,
    ERROR_UNKNOWN
};

// Metal buffer type
enum class MetalBufferType {
    SHARED,
    MANAGED,
    PRIVATE
};

// Metal precision type
enum class MetalPrecision {
    FP32,
    FP16,
    INT32,
    INT64
};

// Metal context class
class MetalContext {
public:
    // Constructor and destructor
    MetalContext();
    ~MetalContext();
    
    // Initialize Metal context
    MetalStatus initialize(int device_id = -1);
    
    // Get device properties
    const system::GPUInfo* getDeviceInfo() const;
    
    // Get Metal status
    MetalStatus getStatus() const;
    
    // Get error message
    std::string getErrorMessage() const;
    
    // Check if Metal is available
    bool isAvailable() const;
    
    // Get device count
    int getDeviceCount() const;
    
    // Set device
    MetalStatus setDevice(int device_id);
    
    // Get current device
    int getCurrentDevice() const;
    
    // Create command queue
    std::shared_ptr<MetalCommandQueue> createCommandQueue();
    
    // Create buffer
    std::shared_ptr<MetalBuffer> createBuffer(size_t size_bytes, MetalBufferType buffer_type = MetalBufferType::SHARED);
    
    // Create compute pipeline
    std::shared_ptr<MetalComputePipeline> createComputePipeline(const std::string& function_name, const std::string& shader_source);
    
    // Load pre-compiled compute pipeline
    std::shared_ptr<MetalComputePipeline> loadComputePipeline(const std::string& function_name, const std::string& library_path);
    
    // Get native device
    void* getNativeDevice() const;
    
private:
    // Metal context state
    MetalStatus status_;
    std::string error_message_;
    int device_count_;
    int current_device_;
    system::GPUInfo device_info_;
    void* metal_device_; // Opaque pointer to MTLDevice
    void* default_library_; // Opaque pointer to MTLLibrary
    
    // Metal API wrappers
    MetalStatus getDeviceCount(int& count);
    MetalStatus getDeviceProperties(int device_id, system::GPUInfo& info);
};

// Metal command queue class
class MetalCommandQueue {
public:
    // Constructor and destructor
    MetalCommandQueue(MetalContext* context);
    ~MetalCommandQueue();
    
    // Create command buffer
    void* createCommandBuffer();
    
    // Get Metal status
    MetalStatus getStatus() const;
    
    // Get error message
    std::string getErrorMessage() const;
    
    // Get native command queue
    void* getNativeCommandQueue() const;
    
private:
    // Metal command queue state
    MetalContext* context_;
    MetalStatus status_;
    std::string error_message_;
    void* command_queue_; // Opaque pointer to MTLCommandQueue
    
    // Friend classes
    friend class MetalComputePipeline;
    friend class MetalBuffer;
};

// Metal buffer class
class MetalBuffer {
public:
    // Constructor and destructor
    MetalBuffer(MetalContext* context, size_t size_bytes, MetalBufferType buffer_type);
    ~MetalBuffer();
    
    // Copy data from host to buffer
    MetalStatus copyFromHost(const void* host_ptr, size_t size_bytes = 0, size_t offset = 0);
    
    // Copy data from buffer to host
    MetalStatus copyToHost(void* host_ptr, size_t size_bytes = 0, size_t offset = 0);
    
    // Copy data from buffer to buffer
    MetalStatus copyFromBuffer(const MetalBuffer& source, size_t size_bytes = 0, size_t dest_offset = 0, size_t source_offset = 0);
    
    // Get pointer (only valid for shared buffers)
    void* getPtr() const;
    
    // Get size
    size_t getSize() const;
    
    // Get buffer type
    MetalBufferType getBufferType() const;
    
    // Get Metal status
    MetalStatus getStatus() const;
    
    // Get error message
    std::string getErrorMessage() const;
    
    // Get native buffer
    void* getNativeBuffer() const;
    
private:
    // Metal buffer state
    MetalContext* context_;
    MetalStatus status_;
    std::string error_message_;
    void* buffer_; // Opaque pointer to MTLBuffer
    void* ptr_; // Direct pointer for shared buffers
    size_t size_bytes_;
    MetalBufferType buffer_type_;
    
    // Friend classes
    friend class MetalComputePipeline;
};

// Metal compute pipeline class
class MetalComputePipeline {
public:
    // Constructor and destructor
    MetalComputePipeline(MetalContext* context, const std::string& function_name);
    ~MetalComputePipeline();
    
    // Compile shader from source
    MetalStatus compile(const std::string& shader_source);
    
    // Load pre-compiled shader
    MetalStatus load(const std::string& library_path);
    
    // Dispatch compute
    MetalStatus dispatch(MetalCommandQueue& queue, size_t grid_width, size_t grid_height = 1, size_t grid_depth = 1);
    
    // Set buffer
    MetalStatus setBuffer(const MetalBuffer& buffer, size_t index);
    
    // Set bytes
    template<typename T>
    MetalStatus setBytes(const T& data, size_t index);
    
    // Get Metal status
    MetalStatus getStatus() const;
    
    // Get error message
    std::string getErrorMessage() const;
    
private:
    // Metal compute pipeline state
    MetalContext* context_;
    MetalStatus status_;
    std::string error_message_;
    std::string function_name_;
    void* compute_pipeline_state_; // Opaque pointer to MTLComputePipelineState
    void* compute_function_; // Opaque pointer to MTLFunction
    
    // Thread group size
    size_t thread_group_size_x_;
    size_t thread_group_size_y_;
    size_t thread_group_size_z_;
    
    // Helper methods
    MetalStatus createComputePipelineState();
};

// MFP Metal implementation class
class MFPMetal {
public:
    // Constructor and destructor
    MFPMetal();
    ~MFPMetal();
    
    // Initialize Metal
    bool initialize(int device_id = -1);
    
    // Check if Metal is available
    bool isAvailable() const;
    
    // Get Metal context
    MetalContext* getContext() const;
    
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
    // Metal state
    std::unique_ptr<MetalContext> context_;
    bool performance_logging_enabled_;
    
    // Performance metrics
    struct PerformanceMetrics {
        double kernel_execution_time_ms;
        double memory_transfer_time_ms;
        double total_time_ms;
        size_t memory_used_bytes;
        size_t grid_width;
        size_t grid_height;
        size_t grid_depth;
        std::string method_name;
    };
    
    std::vector<PerformanceMetrics> performance_metrics_;
    
    // Helper methods
    bool initializeShaders();
    void logPerformance(const PerformanceMetrics& metrics);
    
    // Metal compute pipelines
    std::shared_ptr<MetalComputePipeline> method1_pipeline_;
    std::shared_ptr<MetalComputePipeline> method2_pipeline_;
    std::shared_ptr<MetalComputePipeline> method3_pipeline_;
    std::shared_ptr<MetalComputePipeline> is_prime_pipeline_;
    std::shared_ptr<MetalComputePipeline> find_next_prime_pipeline_;
    
    // Implementation methods
    bool implementMethod1(const std::string& number, std::vector<std::string>& factors);
    bool implementMethod2(const std::string& number, std::vector<std::string>& factors);
    bool implementMethod3(const std::string& number, std::vector<std::string>& factors);
};

} // namespace metal
} // namespace mfp
