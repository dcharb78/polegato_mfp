#pragma once

#include <string>
#include <vector>
#include <memory>
#include <gmp.h>
#include "mfp_base.h"

namespace mfp {

// Forward declarations
class GPUInfo;

// Metal-specific data structures and types
#ifdef __APPLE__
// Forward declarations for Metal types
typedef void* MTLDevice;
typedef void* MTLCommandQueue;
typedef void* MTLBuffer;
typedef void* MTLComputePipelineState;
typedef void* MTLCommandBuffer;
typedef void* MTLComputeCommandEncoder;
#else
// Dummy types for non-Apple platforms
typedef void* MTLDevice;
typedef void* MTLCommandQueue;
typedef void* MTLBuffer;
typedef void* MTLComputePipelineState;
typedef void* MTLCommandBuffer;
typedef void* MTLComputeCommandEncoder;
#endif

// Metal memory management
class MetalMemory {
public:
    MetalMemory();
    ~MetalMemory();
    
    // Initialize Metal memory system
    bool initialize(MTLDevice device, size_t max_memory_bytes = 0);
    
    // Allocate device buffer
    MTLBuffer allocate(size_t size_bytes);
    
    // Free device buffer
    void free(MTLBuffer buffer);
    
    // Copy host to device
    bool copyHostToDevice(MTLBuffer buffer, const void* host_ptr, size_t size_bytes);
    
    // Copy device to host
    bool copyDeviceToHost(void* host_ptr, MTLBuffer buffer, size_t size_bytes);
    
    // Get available memory
    size_t getAvailableMemory() const;
    
    // Get total memory
    size_t getTotalMemory() const;
    
private:
    bool m_initialized;
    MTLDevice m_device;
    size_t m_total_memory;
    size_t m_allocated_memory;
};

// Metal shader management
class MetalShader {
public:
    MetalShader();
    ~MetalShader();
    
    // Load shader from source
    bool loadFromSource(MTLDevice device, const std::string& source, const std::string& function_name);
    
    // Load shader from library
    bool loadFromLibrary(MTLDevice device, const std::string& function_name);
    
    // Get pipeline state
    MTLComputePipelineState getPipelineState() const;
    
    // Get function name
    const std::string& getFunctionName() const;
    
private:
    MTLComputePipelineState m_pipeline_state;
    std::string m_function_name;
};

// Metal command management
class MetalCommand {
public:
    MetalCommand();
    ~MetalCommand();
    
    // Initialize with command queue
    bool initialize(MTLCommandQueue command_queue);
    
    // Begin encoding
    bool beginEncoding();
    
    // Set compute pipeline state
    void setComputePipelineState(MTLComputePipelineState pipeline_state);
    
    // Set buffer
    void setBuffer(MTLBuffer buffer, int index);
    
    // Dispatch compute
    void dispatch(int width, int height = 1, int depth = 1);
    
    // End encoding
    void endEncoding();
    
    // Commit and wait
    bool commitAndWait();
    
private:
    MTLCommandQueue m_command_queue;
    MTLCommandBuffer m_command_buffer;
    MTLComputeCommandEncoder m_compute_encoder;
    bool m_encoding;
};

// Metal accelerator for MFP operations
class MetalAccelerator {
public:
    MetalAccelerator();
    ~MetalAccelerator();
    
    // Initialize Metal accelerator
    bool initialize(const GPUInfo& gpu_info);
    
    // Check if Metal is available
    bool isAvailable() const;
    
    // Get device properties
    std::string getDeviceProperties() const;
    
    // MFP operations
    bool isPrime(const mpz_t number, bool& result);
    bool factorize(const mpz_t number, std::vector<mpz_t>& factors);
    bool nextPrime(const mpz_t number, mpz_t next_prime);
    
    // Performance benchmarking
    double benchmark(const mpz_t number);
    
private:
    bool m_initialized;
    MTLDevice m_device;
    MTLCommandQueue m_command_queue;
    MetalMemory m_memory;
    std::unique_ptr<MetalShader> m_shader_is_prime;
    std::unique_ptr<MetalShader> m_shader_factorize;
    std::unique_ptr<MetalShader> m_shader_next_prime;
    
    // Helper methods
    bool loadShaders();
    bool configureThreadGroups(size_t data_size, int& width, int& height, int& depth);
};

// Factory method to create MFP implementation using Metal
std::unique_ptr<MFPBase> createMetalMFP(int method_number);

} // namespace mfp
