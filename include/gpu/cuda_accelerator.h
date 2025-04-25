#pragma once

#include <string>
#include <vector>
#include <memory>
#include <gmp.h>
#include "mfp_base.h"

namespace mfp {

// Forward declarations
class GPUInfo;

// CUDA-specific data structures and types
#ifdef __CUDA__
typedef struct CUstream_st* cudaStream_t;
typedef struct CUcontext_st* CUcontext;
typedef struct CUmodule_st* CUmodule;
typedef struct CUfunction_st* CUfunction;
#else
typedef void* cudaStream_t;
typedef void* CUcontext;
typedef void* CUmodule;
typedef void* CUfunction;
#endif

// CUDA memory management
class CUDAMemory {
public:
    CUDAMemory();
    ~CUDAMemory();
    
    // Initialize CUDA memory system
    bool initialize(size_t max_memory_bytes = 0);
    
    // Allocate device memory
    void* allocate(size_t size_bytes);
    
    // Free device memory
    void free(void* ptr);
    
    // Copy host to device
    bool copyHostToDevice(void* device_ptr, const void* host_ptr, size_t size_bytes);
    
    // Copy device to host
    bool copyDeviceToHost(void* host_ptr, const void* device_ptr, size_t size_bytes);
    
    // Copy device to device
    bool copyDeviceToDevice(void* dst_ptr, const void* src_ptr, size_t size_bytes);
    
    // Get available memory
    size_t getAvailableMemory() const;
    
    // Get total memory
    size_t getTotalMemory() const;
    
private:
    bool m_initialized;
    size_t m_total_memory;
    size_t m_allocated_memory;
};

// CUDA kernel management
class CUDAKernel {
public:
    CUDAKernel();
    ~CUDAKernel();
    
    // Load kernel from PTX or CUBIN
    bool loadFromFile(const std::string& filename);
    
    // Load kernel from embedded PTX
    bool loadFromPTX(const std::string& ptx_string, const std::string& kernel_name);
    
    // Set kernel parameters
    template<typename T>
    void setParameter(int index, const T& value);
    
    // Set kernel grid and block dimensions
    void setGridDim(int grid_x, int grid_y = 1, int grid_z = 1);
    void setBlockDim(int block_x, int block_y = 1, int block_z = 1);
    
    // Launch kernel
    bool launch(cudaStream_t stream = nullptr);
    
private:
    CUmodule m_module;
    CUfunction m_function;
    std::string m_kernel_name;
    std::vector<void*> m_params;
    std::vector<size_t> m_param_sizes;
    int m_grid_dim[3];
    int m_block_dim[3];
};

// CUDA stream management
class CUDAStream {
public:
    CUDAStream();
    ~CUDAStream();
    
    // Create stream
    bool create();
    
    // Destroy stream
    void destroy();
    
    // Synchronize stream
    bool synchronize();
    
    // Get native stream handle
    cudaStream_t getHandle() const;
    
private:
    cudaStream_t m_stream;
};

// CUDA accelerator for MFP operations
class CUDAAccelerator {
public:
    CUDAAccelerator();
    ~CUDAAccelerator();
    
    // Initialize CUDA accelerator
    bool initialize(const GPUInfo& gpu_info);
    
    // Check if CUDA is available
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
    CUDAMemory m_memory;
    CUDAStream m_stream;
    std::unique_ptr<CUDAKernel> m_kernel_is_prime;
    std::unique_ptr<CUDAKernel> m_kernel_factorize;
    std::unique_ptr<CUDAKernel> m_kernel_next_prime;
    
    // Helper methods
    bool loadKernels();
    bool configureGrids(size_t data_size);
};

// Factory method to create MFP implementation using CUDA
std::unique_ptr<MFPBase> createCUDAMFP(int method_number);

} // namespace mfp
