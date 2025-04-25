#include "cuda_accelerator.h"
#include <chrono>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <iostream>
#include <fstream>

// CUDA runtime includes (if available)
#if defined(HAVE_CUDA)
#include <cuda_runtime.h>
#include <cuda.h>
#include <nvrtc.h>
#endif

namespace mfp {
namespace cuda {

//=============================================================================
// CUDA Context Implementation
//=============================================================================

CUDAContext::CUDAContext() 
    : status_(CUDAStatus::SUCCESS), 
      device_count_(0), 
      current_device_(-1), 
      cuda_context_(nullptr) {
}

CUDAContext::~CUDAContext() {
#if defined(HAVE_CUDA)
    // Clean up CUDA context
    if (cuda_context_) {
        CUcontext context = static_cast<CUcontext>(cuda_context_);
        cuCtxDestroy(context);
        cuda_context_ = nullptr;
    }
#endif
}

CUDAStatus CUDAContext::initialize(int device_id) {
#if defined(HAVE_CUDA)
    // Initialize CUDA runtime
    cudaError_t error = cudaInit(0);
    if (error != cudaSuccess) {
        status_ = CUDAStatus::ERROR_INITIALIZATION;
        error_message_ = cudaGetErrorString(error);
        return status_;
    }
    
    // Get device count
    status_ = getDeviceCount(device_count_);
    if (status_ != CUDAStatus::SUCCESS) {
        return status_;
    }
    
    if (device_count_ == 0) {
        status_ = CUDAStatus::ERROR_INVALID_DEVICE;
        error_message_ = "No CUDA-capable devices found";
        return status_;
    }
    
    // Select device
    if (device_id < 0) {
        // Auto-select best device
        int best_device = 0;
        int max_compute_capability = 0;
        
        for (int i = 0; i < device_count_; ++i) {
            cudaDeviceProp prop;
            error = cudaGetDeviceProperties(&prop, i);
            if (error != cudaSuccess) {
                continue;
            }
            
            int compute_capability = prop.major * 10 + prop.minor;
            if (compute_capability > max_compute_capability) {
                max_compute_capability = compute_capability;
                best_device = i;
            }
        }
        
        device_id = best_device;
    } else if (device_id >= device_count_) {
        status_ = CUDAStatus::ERROR_INVALID_DEVICE;
        error_message_ = "Invalid device ID: " + std::to_string(device_id);
        return status_;
    }
    
    // Set device
    error = cudaSetDevice(device_id);
    if (error != cudaSuccess) {
        status_ = CUDAStatus::ERROR_INVALID_DEVICE;
        error_message_ = cudaGetErrorString(error);
        return status_;
    }
    
    current_device_ = device_id;
    
    // Get device properties
    status_ = getDeviceProperties(current_device_, device_info_);
    if (status_ != CUDAStatus::SUCCESS) {
        return status_;
    }
    
    // Create CUDA context
    CUdevice cu_device;
    CUresult result = cuDeviceGet(&cu_device, current_device_);
    if (result != CUDA_SUCCESS) {
        status_ = CUDAStatus::ERROR_INITIALIZATION;
        error_message_ = "Failed to get CUDA device";
        return status_;
    }
    
    CUcontext context;
    result = cuCtxCreate(&context, 0, cu_device);
    if (result != CUDA_SUCCESS) {
        status_ = CUDAStatus::ERROR_INITIALIZATION;
        error_message_ = "Failed to create CUDA context";
        return status_;
    }
    
    cuda_context_ = static_cast<void*>(context);
    
    return CUDAStatus::SUCCESS;
#else
    status_ = CUDAStatus::ERROR_NOT_IMPLEMENTED;
    error_message_ = "CUDA support not compiled in";
    return status_;
#endif
}

const system::GPUInfo* CUDAContext::getDeviceInfo() const {
    if (current_device_ < 0) {
        return nullptr;
    }
    return &device_info_;
}

CUDAStatus CUDAContext::getStatus() const {
    return status_;
}

std::string CUDAContext::getErrorMessage() const {
    return error_message_;
}

bool CUDAContext::isAvailable() const {
#if defined(HAVE_CUDA)
    return device_count_ > 0 && current_device_ >= 0;
#else
    return false;
#endif
}

int CUDAContext::getDeviceCount() const {
    return device_count_;
}

CUDAStatus CUDAContext::setDevice(int device_id) {
#if defined(HAVE_CUDA)
    if (device_id < 0 || device_id >= device_count_) {
        status_ = CUDAStatus::ERROR_INVALID_DEVICE;
        error_message_ = "Invalid device ID: " + std::to_string(device_id);
        return status_;
    }
    
    cudaError_t error = cudaSetDevice(device_id);
    if (error != cudaSuccess) {
        status_ = CUDAStatus::ERROR_INVALID_DEVICE;
        error_message_ = cudaGetErrorString(error);
        return status_;
    }
    
    current_device_ = device_id;
    
    // Get device properties
    status_ = getDeviceProperties(current_device_, device_info_);
    if (status_ != CUDAStatus::SUCCESS) {
        return status_;
    }
    
    return CUDAStatus::SUCCESS;
#else
    status_ = CUDAStatus::ERROR_NOT_IMPLEMENTED;
    error_message_ = "CUDA support not compiled in";
    return status_;
#endif
}

int CUDAContext::getCurrentDevice() const {
    return current_device_;
}

CUDAStatus CUDAContext::synchronize() {
#if defined(HAVE_CUDA)
    cudaError_t error = cudaDeviceSynchronize();
    if (error != cudaSuccess) {
        status_ = CUDAStatus::ERROR_SYNCHRONIZATION;
        error_message_ = cudaGetErrorString(error);
        return status_;
    }
    
    return CUDAStatus::SUCCESS;
#else
    status_ = CUDAStatus::ERROR_NOT_IMPLEMENTED;
    error_message_ = "CUDA support not compiled in";
    return status_;
#endif
}

std::shared_ptr<CUDAStream> CUDAContext::createStream() {
    return std::make_shared<CUDAStream>(this);
}

std::shared_ptr<CUDAMemory> CUDAContext::allocateMemory(size_t size_bytes, CUDAMemoryType memory_type) {
    return std::make_shared<CUDAMemory>(this, size_bytes, memory_type);
}

std::shared_ptr<CUDAKernel> CUDAContext::createKernel(const std::string& kernel_name, const std::string& kernel_source) {
    auto kernel = std::make_shared<CUDAKernel>(this, kernel_name);
    kernel->compile(kernel_source);
    return kernel;
}

std::shared_ptr<CUDAKernel> CUDAContext::loadKernel(const std::string& kernel_name, const std::string& ptx_file) {
    auto kernel = std::make_shared<CUDAKernel>(this, kernel_name);
    kernel->load(ptx_file);
    return kernel;
}

CUDAStatus CUDAContext::getDeviceCount(int& count) {
#if defined(HAVE_CUDA)
    cudaError_t error = cudaGetDeviceCount(&count);
    if (error != cudaSuccess) {
        status_ = CUDAStatus::ERROR_INITIALIZATION;
        error_message_ = cudaGetErrorString(error);
        return status_;
    }
    
    return CUDAStatus::SUCCESS;
#else
    count = 0;
    status_ = CUDAStatus::ERROR_NOT_IMPLEMENTED;
    error_message_ = "CUDA support not compiled in";
    return status_;
#endif
}

CUDAStatus CUDAContext::getDeviceProperties(int device_id, system::GPUInfo& info) {
#if defined(HAVE_CUDA)
    cudaDeviceProp prop;
    cudaError_t error = cudaGetDeviceProperties(&prop, device_id);
    if (error != cudaSuccess) {
        status_ = CUDAStatus::ERROR_INVALID_DEVICE;
        error_message_ = cudaGetErrorString(error);
        return status_;
    }
    
    // Fill GPU info
    info.device_id = device_id;
    info.name = prop.name;
    info.vendor = system::GPUVendor::NVIDIA;
    
    // Determine architecture based on compute capability
    std::string compute_capability = std::to_string(prop.major) + "." + std::to_string(prop.minor);
    if (prop.major == 3) {
        info.architecture = system::GPUArchitecture::KEPLER;
    } else if (prop.major == 5) {
        info.architecture = system::GPUArchitecture::MAXWELL;
    } else if (prop.major == 6) {
        info.architecture = system::GPUArchitecture::PASCAL;
    } else if (prop.major == 7) {
        if (prop.minor == 0) {
            info.architecture = system::GPUArchitecture::VOLTA;
        } else {
            info.architecture = system::GPUArchitecture::TURING;
        }
    } else if (prop.major == 8) {
        info.architecture = system::GPUArchitecture::AMPERE;
    } else if (prop.major == 9) {
        if (prop.minor == 0) {
            info.architecture = system::GPUArchitecture::ADA_LOVELACE;
        } else {
            info.architecture = system::GPUArchitecture::HOPPER;
        }
    } else {
        info.architecture = system::GPUArchitecture::UNKNOWN;
    }
    
    // Add CUDA API support
    info.api_support.push_back(system::GPUAPISupport::CUDA);
    
    // Memory information
    info.memory_info.total_memory_bytes = prop.totalGlobalMem;
    info.memory_info.memory_bus_width = prop.memoryBusWidth;
    info.memory_info.memory_clock_mhz = prop.memoryClockRate / 1000.0;
    
    // Calculate memory bandwidth
    info.memory_info.memory_bandwidth_gbps = 2.0 * info.memory_info.memory_clock_mhz * (info.memory_info.memory_bus_width / 8) / 1000.0;
    
    // Compute information
    info.compute_info.cuda_cores = prop.multiProcessorCount * _ConvertSMVer2Cores(prop.major, prop.minor);
    info.compute_info.core_clock_mhz = prop.clockRate / 1000.0;
    info.compute_info.cuda_compute_capability = compute_capability;
    
    // PCIe information
    info.pcie_generation = prop.pciDeviceID >> 16;
    info.pcie_lanes = prop.pciDeviceID & 0xFFFF;
    
    // Check if integrated
    info.is_integrated = (prop.integrated != 0);
    
    // Calculate theoretical performance
    double ops_per_cycle = info.compute_info.cuda_cores * 2; // FMA = 2 operations per cycle
    info.compute_info.theoretical_tflops_fp32 = ops_per_cycle * info.compute_info.core_clock_mhz / 1e6;
    
    return CUDAStatus::SUCCESS;
#else
    status_ = CUDAStatus::ERROR_NOT_IMPLEMENTED;
    error_message_ = "CUDA support not compiled in";
    return status_;
#endif
}

//=============================================================================
// CUDA Stream Implementation
//=============================================================================

CUDAStream::CUDAStream(CUDAContext* context) 
    : context_(context), 
      status_(CUDAStatus::SUCCESS), 
      stream_(nullptr) {
#if defined(HAVE_CUDA)
    cudaStream_t stream;
    cudaError_t error = cudaStreamCreate(&stream);
    if (error != cudaSuccess) {
        status_ = CUDAStatus::ERROR_INITIALIZATION;
        error_message_ = cudaGetErrorString(error);
        return;
    }
    
    stream_ = static_cast<void*>(stream);
#else
    status_ = CUDAStatus::ERROR_NOT_IMPLEMENTED;
    error_message_ = "CUDA support not compiled in";
#endif
}

CUDAStream::~CUDAStream() {
#if defined(HAVE_CUDA)
    if (stream_) {
        cudaStreamDestroy(static_cast<cudaStream_t>(stream_));
        stream_ = nullptr;
    }
#endif
}

CUDAStatus CUDAStream::synchronize() {
#if defined(HAVE_CUDA)
    if (!stream_) {
        status_ = CUDAStatus::ERROR_INITIALIZATION;
        error_message_ = "Stream not initialized";
        return status_;
    }
    
    cudaError_t error = cudaStreamSynchronize(static_cast<cudaStream_t>(stream_));
    if (error != cudaSuccess) {
        status_ = CUDAStatus::ERROR_SYNCHRONIZATION;
        error_message_ = cudaGetErrorString(error);
        return status_;
    }
    
    return CUDAStatus::SUCCESS;
#else
    status_ = CUDAStatus::ERROR_NOT_IMPLEMENTED;
    error_message_ = "CUDA support not compiled in";
    return status_;
#endif
}

CUDAStatus CUDAStream::getStatus() const {
    return status_;
}

std::string CUDAStream::getErrorMessage() const {
    return error_message_;
}

void* CUDAStream::getNativeHandle() const {
    return stream_;
}

//=============================================================================
// CUDA Memory Implementation
//=============================================================================

CUDAMemory::CUDAMemory(CUDAContext* context, size_t size_bytes, CUDAMemoryType memory_type) 
    : context_(context), 
      status_(CUDAStatus::SUCCESS), 
      ptr_(nullptr), 
      size_bytes_(size_bytes), 
      memory_type_(memory_type) {
#if defined(HAVE_CUDA)
    cudaError_t error = cudaSuccess;
    
    switch (memory_type) {
        case CUDAMemoryType::HOST:
            error = cudaMallocHost(&ptr_, size_bytes);
            break;
            
        case CUDAMemoryType::DEVICE:
            error = cudaMalloc(&ptr_, size_bytes);
            break;
            
        case CUDAMemoryType::UNIFIED:
            error = cudaMallocManaged(&ptr_, size_bytes);
            break;
            
        case CUDAMemoryType::PINNED:
            error = cudaHostAlloc(&ptr_, size_bytes, cudaHostAllocDefault);
            break;
    }
    
    if (error != cudaSuccess) {
        status_ = CUDAStatus::ERROR_MEMORY_ALLOCATION;
        error_message_ = cudaGetErrorString(error);
        ptr_ = nullptr;
        size_bytes_ = 0;
    }
#else
    status_ = CUDAStatus::ERROR_NOT_IMPLEMENTED;
    error_message_ = "CUDA support not compiled in";
#endif
}

CUDAMemory::~CUDAMemory() {
#if defined(HAVE_CUDA)
    if (ptr_) {
        cudaError_t error = cudaSuccess;
        
        switch (memory_type_) {
            case CUDAMemoryType::HOST:
            case CUDAMemoryType::PINNED:
                error = cudaFreeHost(ptr_);
                break;
                
            case CUDAMemoryType::DEVICE:
            case CUDAMemoryType::UNIFIED:
                error = cudaFree(ptr_);
                break;
        }
        
        ptr_ = nullptr;
        size_bytes_ = 0;
    }
#endif
}

CUDAStatus CUDAMemory::copyFromHost(const void* host_ptr, size_t size_bytes, size_t offset) {
#if defined(HAVE_CUDA)
    if (!ptr_) {
        status_ = CUDAStatus::ERROR_MEMORY_ALLOCATION;
        error_message_ = "Memory not allocated";
        return status_;
    }
    
    if (!host_ptr) {
        status_ = CUDAStatus::ERROR_MEMORY_COPY;
        error_message_ = "Invalid host pointer";
        return status_;
    }
    
    if (size_bytes == 0) {
        size_bytes = size_bytes_ - offset;
    }
    
    if (offset + size_bytes > size_bytes_) {
        status_ = CUDAStatus::ERROR_MEMORY_COPY;
        error_message_ = "Copy size exceeds allocated memory";
        return status_;
    }
    
    cudaError_t error = cudaMemcpy(static_cast<char*>(ptr_) + offset, host_ptr, size_bytes, cudaMemcpyHostToDevice);
    if (error != cudaSuccess) {
        status_ = CUDAStatus::ERROR_MEMORY_COPY;
        error_message_ = cudaGetErrorString(error);
        return status_;
    }
    
    return CUDAStatus::SUCCESS;
#else
    status_ = CUDAStatus::ERROR_NOT_IMPLEMENTED;
    error_message_ = "CUDA support not compiled in";
    return status_;
#endif
}

CUDAStatus CUDAMemory::copyToHost(void* host_ptr, size_t size_bytes, size_t offset) {
#if defined(HAVE_CUDA)
    if (!ptr_) {
        status_ = CUDAStatus::ERROR_MEMORY_ALLOCATION;
        error_message_ = "Memory not allocated";
        return status_;
    }
    
    if (!host_ptr) {
        status_ = CUDAStatus::ERROR_MEMORY_COPY;
        error_message_ = "Invalid host pointer";
        return status_;
    }
    
    if (size_bytes == 0) {
        size_bytes = size_bytes_ - offset;
    }
    
    if (offset + size_bytes > size_bytes_) {
        status_ = CUDAStatus::ERROR_MEMORY_COPY;
        error_message_ = "Copy size exceeds allocated memory";
        return status_;
    }
    
    cudaError_t error = cudaMemcpy(host_ptr, static_cast<char*>(ptr_) + offset, size_bytes, cudaMemcpyDeviceToHost);
    if (error != cudaSuccess) {
        status_ = CUDAStatus::ERROR_MEMORY_COPY;
        error_message_ = cudaGetErrorString(error);
        return status_;
    }
    
    return CUDAStatus::SUCCESS;
#else
    status_ = CUDAStatus::ERROR_NOT_IMPLEMENTED;
    error_message_ = "CUDA support not compiled in";
    return status_;
#endif
}

CUDAStatus CUDAMemory::copyFromDevice(const CUDAMemory& source, size_t size_bytes, size_t dest_offset, size_t source_offset) {
#if defined(HAVE_CUDA)
    if (!ptr_) {
        status_ = CUDAStatus::ERROR_MEMORY_ALLOCATION;
        error_message_ = "Destination memory not allocated";
        return status_;
    }
    
    if (!source.ptr_) {
        status_ = CUDAStatus::ERROR_MEMORY_ALLOCATION;
        error_message_ = "Source memory not allocated";
        return status_;
    }
    
    if (size_bytes == 0) {
        size_bytes = std::min(size_bytes_ - dest_offset, source.size_bytes_ - source_offset);
    }
    
    if (dest_offset + size_bytes > size_bytes_) {
        status_ = CUDAStatus::ERROR_MEMORY_COPY;
        error_message_ = "Copy size exceeds destination allocated memory";
        return status_;
    }
    
    if (source_offset + size_bytes > source.size_bytes_) {
        status_ = CUDAStatus::ERROR_MEMORY_COPY;
        error_message_ = "Copy size exceeds source allocated memory";
        return status_;
    }
    
    cudaError_t error = cudaMemcpy(
        static_cast<char*>(ptr_) + dest_offset, 
        static_cast<char*>(source.ptr_) + source_offset, 
        size_bytes, 
        cudaMemcpyDeviceToDevice
    );
    
    if (error != cudaSuccess) {
        status_ = CUDAStatus::ERROR_MEMORY_COPY;
        error_message_ = cudaGetErrorString(error);
        return status_;
    }
    
    return CUDAStatus::SUCCESS;
#else
    status_ = CUDAStatus::ERROR_NOT_IMPLEMENTED;
    error_message_ = "CUDA support not compiled in";
    return status_;
#endif
}

CUDAStatus CUDAMemory::copyFromHostAsync(const void* host_ptr, CUDAStream& stream, size_t size_bytes, size_t offset) {
#if defined(HAVE_CUDA)
    if (!ptr_) {
        status_ = CUDAStatus::ERROR_MEMORY_ALLOCATION;
        error_message_ = "Memory not allocated";
        return status_;
    }
    
    if (!host_ptr) {
        status_ = CUDAStatus::ERROR_MEMORY_COPY;
        error_message_ = "Invalid host pointer";
        return status_;
    }
    
    if (!stream.stream_) {
        status_ = CUDAStatus::ERROR_INITIALIZATION;
        error_message_ = "Stream not initialized";
        return status_;
    }
    
    if (size_bytes == 0) {
        size_bytes = size_bytes_ - offset;
    }
    
    if (offset + size_bytes > size_bytes_) {
        status_ = CUDAStatus::ERROR_MEMORY_COPY;
        error_message_ = "Copy size exceeds allocated memory";
        return status_;
    }
    
    cudaError_t error = cudaMemcpyAsync(
        static_cast<char*>(ptr_) + offset, 
        host_ptr, 
        size_bytes, 
        cudaMemcpyHostToDevice, 
        static_cast<cudaStream_t>(stream.stream_)
    );
    
    if (error != cudaSuccess) {
        status_ = CUDAStatus::ERROR_MEMORY_COPY;
        error_message_ = cudaGetErrorString(error);
        return status_;
    }
    
    return CUDAStatus::SUCCESS;
#else
    status_ = CUDAStatus::ERROR_NOT_IMPLEMENTED;
    error_message_ = "CUDA support not compiled in";
    return status_;
#endif
}

CUDAStatus CUDAMemory::copyToHostAsync(void* host_ptr, CUDAStream& stream, size_t size_bytes, size_t offset) {
#if defined(HAVE_CUDA)
    if (!ptr_) {
        status_ = CUDAStatus::ERROR_MEMORY_ALLOCATION;
        error_message_ = "Memory not allocated";
        return status_;
    }
    
    if (!host_ptr) {
        status_ = CUDAStatus::ERROR_MEMORY_COPY;
        error_message_ = "Invalid host pointer";
        return status_;
    }
    
    if (!stream.stream_) {
        status_ = CUDAStatus::ERROR_INITIALIZATION;
        error_message_ = "Stream not initialized";
        return status_;
    }
    
    if (size_bytes == 0) {
        size_bytes = size_bytes_ - offset;
    }
    
    if (offset + size_bytes > size_bytes_) {
        status_ = CUDAStatus::ERROR_MEMORY_COPY;
        error_message_ = "Copy size exceeds allocated memory";
        return status_;
    }
    
    cudaError_t error = cudaMemcpyAsync(
        host_ptr, 
        static_cast<char*>(ptr_) + offset, 
        size_bytes, 
        cudaMemcpyDeviceToHost, 
        static_cast<cudaStream_t>(stream.stream_)
    );
    
    if (error != cudaSuccess) {
        status_ = CUDAStatus::ERROR_MEMORY_COPY;
        error_message_ = cudaGetErrorString(error);
        return status_;
    }
    
    return CUDAStatus::SUCCESS;
#else
    status_ = CUDAStatus::ERROR_NOT_IMPLEMENTED;
    error_message_ = "CUDA support not compiled in";
    return status_;
#endif
}

CUDAStatus CUDAMemory::copyFromDeviceAsync(const CUDAMemory& source, CUDAStream& stream, size_t size_bytes, size_t dest_offset, size_t source_offset) {
#if defined(HAVE_CUDA)
    if (!ptr_) {
        status_ = CUDAStatus::ERROR_MEMORY_ALLOCATION;
        error_message_ = "Destination memory not allocated";
        return status_;
    }
    
    if (!source.ptr_) {
        status_ = CUDAStatus::ERROR_MEMORY_ALLOCATION;
        error_message_ = "Source memory not allocated";
        return status_;
    }
    
    if (!stream.stream_) {
        status_ = CUDAStatus::ERROR_INITIALIZATION;
        error_message_ = "Stream not initialized";
        return status_;
    }
    
    if (size_bytes == 0) {
        size_bytes = std::min(size_bytes_ - dest_offset, source.size_bytes_ - source_offset);
    }
    
    if (dest_offset + size_bytes > size_bytes_) {
        status_ = CUDAStatus::ERROR_MEMORY_COPY;
        error_message_ = "Copy size exceeds destination allocated memory";
        return status_;
    }
    
    if (source_offset + size_bytes > source.size_bytes_) {
        status_ = CUDAStatus::ERROR_MEMORY_COPY;
        error_message_ = "Copy size exceeds source allocated memory";
        return status_;
    }
    
    cudaError_t error = cudaMemcpyAsync(
        static_cast<char*>(ptr_) + dest_offset, 
        static_cast<char*>(source.ptr_) + source_offset, 
        size_bytes, 
        cudaMemcpyDeviceToDevice, 
        static_cast<cudaStream_t>(stream.stream_)
    );
    
    if (error != cudaSuccess) {
        status_ = CUDAStatus::ERROR_MEMORY_COPY;
        error_message_ = cudaGetErrorString(error);
        return status_;
    }
    
    return CUDAStatus::SUCCESS;
#else
    status_ = CUDAStatus::ERROR_NOT_IMPLEMENTED;
    error_message_ = "CUDA support not compiled in";
    return status_;
#endif
}

CUDAStatus CUDAMemory::memset(int value, size_t size_bytes, size_t offset) {
#if defined(HAVE_CUDA)
    if (!ptr_) {
        status_ = CUDAStatus::ERROR_MEMORY_ALLOCATION;
        error_message_ = "Memory not allocated";
        return status_;
    }
    
    if (size_bytes == 0) {
        size_bytes = size_bytes_ - offset;
    }
    
    if (offset + size_bytes > size_bytes_) {
        status_ = CUDAStatus::ERROR_MEMORY_COPY;
        error_message_ = "Memset size exceeds allocated memory";
        return status_;
    }
    
    cudaError_t error = cudaMemset(static_cast<char*>(ptr_) + offset, value, size_bytes);
    if (error != cudaSuccess) {
        status_ = CUDAStatus::ERROR_MEMORY_COPY;
        error_message_ = cudaGetErrorString(error);
        return status_;
    }
    
    return CUDAStatus::SUCCESS;
#else
    status_ = CUDAStatus::ERROR_NOT_IMPLEMENTED;
    error_message_ = "CUDA support not compiled in";
    return status_;
#endif
}

CUDAStatus CUDAMemory::memsetAsync(int value, CUDAStream& stream, size_t size_bytes, size_t offset) {
#if defined(HAVE_CUDA)
    if (!ptr_) {
        status_ = CUDAStatus::ERROR_MEMORY_ALLOCATION;
        error_message_ = "Memory not allocated";
        return status_;
    }
    
    if (!stream.stream_) {
        status_ = CUDAStatus::ERROR_INITIALIZATION;
        error_message_ = "Stream not initialized";
        return status_;
    }
    
    if (size_bytes == 0) {
        size_bytes = size_bytes_ - offset;
    }
    
    if (offset + size_bytes > size_bytes_) {
        status_ = CUDAStatus::ERROR_MEMORY_COPY;
        error_message_ = "Memset size exceeds allocated memory";
        return status_;
    }
    
    cudaError_t error = cudaMemsetAsync(
        static_cast<char*>(ptr_) + offset, 
        value, 
        size_bytes, 
        static_cast<cudaStream_t>(stream.stream_)
    );
    
    if (error != cudaSuccess) {
        status_ = CUDAStatus::ERROR_MEMORY_COPY;
        error_message_ = cudaGetErrorString(error);
        return status_;
    }
    
    return CUDAStatus::SUCCESS;
#else
    status_ = CUDAStatus::ERROR_NOT_IMPLEMENTED;
    error_message_ = "CUDA support not compiled in";
    return status_;
#endif
}

void* CUDAMemory::getPtr() const {
    return ptr_;
}

size_t CUDAMemory::getSize() const {
    return size_bytes_;
}

CUDAMemoryType CUDAMemory::getMemoryType() const {
    return memory_type_;
}

CUDAStatus CUDAMemory::getStatus() const {
    return status_;
}

std::string CUDAMemory::getErrorMessage() const {
    return error_message_;
}

//=============================================================================
// CUDA Kernel Implementation
//=============================================================================

CUDAKernel::CUDAKernel(CUDAContext* context, const std::string& kernel_name) 
    : context_(context), 
      status_(CUDAStatus::SUCCESS), 
      kernel_name_(kernel_name), 
      module_(nullptr), 
      function_(nullptr) {
}

CUDAKernel::~CUDAKernel() {
#if defined(HAVE_CUDA)
    if (module_) {
        CUmodule module = static_cast<CUmodule>(module_);
        cuModuleUnload(module);
        module_ = nullptr;
        function_ = nullptr;
    }
#endif
}

CUDAStatus CUDAKernel::compile(const std::string& kernel_source) {
#if defined(HAVE_CUDA) && defined(HAVE_NVRTC)
    // Create NVRTC program
    nvrtcProgram prog;
    nvrtcResult result = nvrtcCreateProgram(&prog, kernel_source.c_str(), (kernel_name_ + ".cu").c_str(), 0, NULL, NULL);
    if (result != NVRTC_SUCCESS) {
        status_ = CUDAStatus::ERROR_INITIALIZATION;
        error_message_ = "Failed to create NVRTC program";
        return status_;
    }
    
    // Get compute capability of current device
    const system::GPUInfo* device_info = context_->getDeviceInfo();
    if (!device_info) {
        status_ = CUDAStatus::ERROR_INITIALIZATION;
        error_message_ = "Failed to get device information";
        nvrtcDestroyProgram(&prog);
        return status_;
    }
    
    std::string compute_capability = device_info->compute_info.cuda_compute_capability;
    std::string arch_flag = "--gpu-architecture=compute_" + compute_capability;
    arch_flag.erase(std::remove(arch_flag.begin(), arch_flag.end(), '.'), arch_flag.end());
    
    // Compile program
    const char* opts[] = {arch_flag.c_str(), "--use_fast_math", "--relocatable-device-code=true"};
    result = nvrtcCompileProgram(prog, 3, opts);
    
    // Get compilation log
    size_t log_size;
    nvrtcGetProgramLogSize(prog, &log_size);
    std::string log(log_size, '\0');
    nvrtcGetProgramLog(prog, &log[0]);
    
    if (result != NVRTC_SUCCESS) {
        status_ = CUDAStatus::ERROR_INITIALIZATION;
        error_message_ = "Failed to compile NVRTC program: " + log;
        nvrtcDestroyProgram(&prog);
        return status_;
    }
    
    // Get PTX
    size_t ptx_size;
    nvrtcGetPTXSize(prog, &ptx_size);
    std::string ptx(ptx_size, '\0');
    nvrtcGetPTX(prog, &ptx[0]);
    
    // Clean up NVRTC program
    nvrtcDestroyProgram(&prog);
    
    // Load PTX
    CUmodule module;
    CUresult cu_result = cuModuleLoadData(&module, ptx.c_str());
    if (cu_result != CUDA_SUCCESS) {
        status_ = CUDAStatus::ERROR_INITIALIZATION;
        error_message_ = "Failed to load PTX module";
        return status_;
    }
    
    module_ = static_cast<void*>(module);
    
    // Get function
    CUfunction function;
    cu_result = cuModuleGetFunction(&function, module, kernel_name_.c_str());
    if (cu_result != CUDA_SUCCESS) {
        status_ = CUDAStatus::ERROR_INITIALIZATION;
        error_message_ = "Failed to get kernel function: " + kernel_name_;
        cuModuleUnload(module);
        module_ = nullptr;
        return status_;
    }
    
    function_ = static_cast<void*>(function);
    
    return CUDAStatus::SUCCESS;
#else
    status_ = CUDAStatus::ERROR_NOT_IMPLEMENTED;
    error_message_ = "CUDA NVRTC support not compiled in";
    return status_;
#endif
}

CUDAStatus CUDAKernel::load(const std::string& ptx_file) {
#if defined(HAVE_CUDA)
    // Read PTX file
    std::ifstream file(ptx_file, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        status_ = CUDAStatus::ERROR_INITIALIZATION;
        error_message_ = "Failed to open PTX file: " + ptx_file;
        return status_;
    }
    
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::string ptx(size, '\0');
    file.read(&ptx[0], size);
    file.close();
    
    // Load PTX
    CUmodule module;
    CUresult result = cuModuleLoadData(&module, ptx.c_str());
    if (result != CUDA_SUCCESS) {
        status_ = CUDAStatus::ERROR_INITIALIZATION;
        error_message_ = "Failed to load PTX module";
        return status_;
    }
    
    module_ = static_cast<void*>(module);
    
    // Get function
    CUfunction function;
    result = cuModuleGetFunction(&function, module, kernel_name_.c_str());
    if (result != CUDA_SUCCESS) {
        status_ = CUDAStatus::ERROR_INITIALIZATION;
        error_message_ = "Failed to get kernel function: " + kernel_name_;
        cuModuleUnload(module);
        module_ = nullptr;
        return status_;
    }
    
    function_ = static_cast<void*>(function);
    
    return CUDAStatus::SUCCESS;
#else
    status_ = CUDAStatus::ERROR_NOT_IMPLEMENTED;
    error_message_ = "CUDA support not compiled in";
    return status_;
#endif
}

template<typename... Args>
CUDAStatus CUDAKernel::setArgs(Args&&... args) {
    args_.clear();
    addArgs(std::forward<Args>(args)...);
    return CUDAStatus::SUCCESS;
}

CUDAStatus CUDAKernel::launch(dim3 grid_dim, dim3 block_dim, size_t shared_memory_bytes) {
#if defined(HAVE_CUDA)
    if (!function_) {
        status_ = CUDAStatus::ERROR_INITIALIZATION;
        error_message_ = "Kernel function not initialized";
        return status_;
    }
    
    CUfunction function = static_cast<CUfunction>(function_);
    
    // Launch kernel
    CUresult result = cuLaunchKernel(
        function,
        grid_dim.x, grid_dim.y, grid_dim.z,
        block_dim.x, block_dim.y, block_dim.z,
        shared_memory_bytes,
        NULL,
        args_.data(),
        NULL
    );
    
    if (result != CUDA_SUCCESS) {
        status_ = CUDAStatus::ERROR_KERNEL_LAUNCH;
        error_message_ = "Failed to launch kernel";
        return status_;
    }
    
    // Synchronize
    result = cuCtxSynchronize();
    if (result != CUDA_SUCCESS) {
        status_ = CUDAStatus::ERROR_SYNCHRONIZATION;
        error_message_ = "Failed to synchronize after kernel launch";
        return status_;
    }
    
    return CUDAStatus::SUCCESS;
#else
    status_ = CUDAStatus::ERROR_NOT_IMPLEMENTED;
    error_message_ = "CUDA support not compiled in";
    return status_;
#endif
}

CUDAStatus CUDAKernel::launchAsync(dim3 grid_dim, dim3 block_dim, CUDAStream& stream, size_t shared_memory_bytes) {
#if defined(HAVE_CUDA)
    if (!function_) {
        status_ = CUDAStatus::ERROR_INITIALIZATION;
        error_message_ = "Kernel function not initialized";
        return status_;
    }
    
    if (!stream.stream_) {
        status_ = CUDAStatus::ERROR_INITIALIZATION;
        error_message_ = "Stream not initialized";
        return status_;
    }
    
    CUfunction function = static_cast<CUfunction>(function_);
    CUstream cu_stream = static_cast<CUstream>(stream.stream_);
    
    // Launch kernel
    CUresult result = cuLaunchKernel(
        function,
        grid_dim.x, grid_dim.y, grid_dim.z,
        block_dim.x, block_dim.y, block_dim.z,
        shared_memory_bytes,
        cu_stream,
        args_.data(),
        NULL
    );
    
    if (result != CUDA_SUCCESS) {
        status_ = CUDAStatus::ERROR_KERNEL_LAUNCH;
        error_message_ = "Failed to launch kernel asynchronously";
        return status_;
    }
    
    return CUDAStatus::SUCCESS;
#else
    status_ = CUDAStatus::ERROR_NOT_IMPLEMENTED;
    error_message_ = "CUDA support not compiled in";
    return status_;
#endif
}

CUDAStatus CUDAKernel::getStatus() const {
    return status_;
}

std::string CUDAKernel::getErrorMessage() const {
    return error_message_;
}

template<typename T>
void CUDAKernel::addArg(T arg) {
    void* arg_ptr = new T(arg);
    args_.push_back(arg_ptr);
}

template<typename T, typename... Args>
void CUDAKernel::addArgs(T first, Args&&... rest) {
    addArg(first);
    addArgs(std::forward<Args>(rest)...);
}

void CUDAKernel::addArgs() {
    // Base case for variadic template recursion
}

//=============================================================================
// MFP CUDA Implementation
//=============================================================================

MFPCUDA::MFPCUDA() 
    : performance_logging_enabled_(false) {
    context_ = std::make_unique<CUDAContext>();
}

MFPCUDA::~MFPCUDA() {
    // Clean up resources
}

bool MFPCUDA::initialize(int device_id) {
    // Initialize CUDA context
    CUDAStatus status = context_->initialize(device_id);
    if (status != CUDAStatus::SUCCESS) {
        return false;
    }
    
    // Initialize kernels
    return initializeKernels();
}

bool MFPCUDA::isAvailable() const {
    return context_->isAvailable();
}

CUDAContext* MFPCUDA::getContext() const {
    return context_.get();
}

const system::GPUInfo* MFPCUDA::getDeviceInfo() const {
    return context_->getDeviceInfo();
}

bool MFPCUDA::runMethod1(const std::string& number, std::vector<std::string>& factors) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    bool result = implementMethod1(number, factors);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    
    if (performance_logging_enabled_) {
        PerformanceMetrics metrics;
        metrics.total_time_ms = duration;
        metrics.method_name = "Method1_ExpandedQFactorization";
        logPerformance(metrics);
    }
    
    return result;
}

bool MFPCUDA::runMethod2(const std::string& number, std::vector<std::string>& factors) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    bool result = implementMethod2(number, factors);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    
    if (performance_logging_enabled_) {
        PerformanceMetrics metrics;
        metrics.total_time_ms = duration;
        metrics.method_name = "Method2_UltrafastWithStructuralFilter";
        logPerformance(metrics);
    }
    
    return result;
}

bool MFPCUDA::runMethod3(const std::string& number, std::vector<std::string>& factors) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    bool result = implementMethod3(number, factors);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    
    if (performance_logging_enabled_) {
        PerformanceMetrics metrics;
        metrics.total_time_ms = duration;
        metrics.method_name = "Method3_ParallelizedWithDynamicBlocks";
        logPerformance(metrics);
    }
    
    return result;
}

bool MFPCUDA::isPrime(const std::string& number) {
    if (!is_prime_kernel_) {
        return false;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Allocate memory for input number
    size_t number_size = number.size();
    auto device_number = context_->allocateMemory(number_size + 1);
    if (device_number->getStatus() != CUDAStatus::SUCCESS) {
        return false;
    }
    
    // Copy number to device
    device_number->copyFromHost(number.c_str(), number_size + 1);
    
    // Allocate memory for result
    auto device_result = context_->allocateMemory(sizeof(int));
    if (device_result->getStatus() != CUDAStatus::SUCCESS) {
        return false;
    }
    
    // Set result to 0 (not prime)
    int result_value = 0;
    device_result->copyFromHost(&result_value, sizeof(int));
    
    // Set kernel arguments
    is_prime_kernel_->setArgs(device_number->getPtr(), number_size, device_result->getPtr());
    
    // Determine grid and block dimensions
    const system::GPUInfo* device_info = context_->getDeviceInfo();
    int threads_per_block = 256;
    int blocks = (device_info->compute_info.cuda_cores + threads_per_block - 1) / threads_per_block;
    blocks = std::min(blocks, 65535); // Maximum grid dimension
    
    // Launch kernel
    dim3 grid_dim(blocks);
    dim3 block_dim(threads_per_block);
    is_prime_kernel_->launch(grid_dim, block_dim);
    
    // Copy result back
    device_result->copyToHost(&result_value, sizeof(int));
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    
    if (performance_logging_enabled_) {
        PerformanceMetrics metrics;
        metrics.total_time_ms = duration;
        metrics.method_name = "IsPrime";
        metrics.blocks_used = blocks;
        metrics.threads_per_block = threads_per_block;
        logPerformance(metrics);
    }
    
    return result_value != 0;
}

std::string MFPCUDA::findNextPrime(const std::string& number) {
    if (!find_next_prime_kernel_) {
        return "";
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Allocate memory for input number
    size_t number_size = number.size();
    auto device_number = context_->allocateMemory(number_size + 1);
    if (device_number->getStatus() != CUDAStatus::SUCCESS) {
        return "";
    }
    
    // Copy number to device
    device_number->copyFromHost(number.c_str(), number_size + 1);
    
    // Allocate memory for result (next prime)
    // Allocate extra space for potential growth
    size_t result_size = number_size + 100;
    auto device_result = context_->allocateMemory(result_size);
    if (device_result->getStatus() != CUDAStatus::SUCCESS) {
        return "";
    }
    
    // Set kernel arguments
    find_next_prime_kernel_->setArgs(device_number->getPtr(), number_size, device_result->getPtr(), result_size);
    
    // Determine grid and block dimensions
    const system::GPUInfo* device_info = context_->getDeviceInfo();
    int threads_per_block = 256;
    int blocks = (device_info->compute_info.cuda_cores + threads_per_block - 1) / threads_per_block;
    blocks = std::min(blocks, 65535); // Maximum grid dimension
    
    // Launch kernel
    dim3 grid_dim(blocks);
    dim3 block_dim(threads_per_block);
    find_next_prime_kernel_->launch(grid_dim, block_dim);
    
    // Copy result back
    std::vector<char> result_buffer(result_size);
    device_result->copyToHost(result_buffer.data(), result_size);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    
    if (performance_logging_enabled_) {
        PerformanceMetrics metrics;
        metrics.total_time_ms = duration;
        metrics.method_name = "FindNextPrime";
        metrics.blocks_used = blocks;
        metrics.threads_per_block = threads_per_block;
        logPerformance(metrics);
    }
    
    return std::string(result_buffer.data());
}

bool MFPCUDA::findPrimeFactors(const std::string& number, std::vector<std::string>& factors) {
    // Use Method 3 (Parallelized with Dynamic Blocks) by default
    return runMethod3(number, factors);
}

void MFPCUDA::setPerformanceLogging(bool enable) {
    performance_logging_enabled_ = enable;
    
    if (!enable) {
        // Clear performance metrics
        performance_metrics_.clear();
    }
}

std::string MFPCUDA::getPerformanceMetrics() const {
    if (performance_metrics_.empty()) {
        return "No performance metrics available";
    }
    
    std::stringstream ss;
    ss << "Performance Metrics:\n";
    
    for (const auto& metrics : performance_metrics_) {
        ss << "Method: " << metrics.method_name << "\n";
        ss << "  Total Time: " << metrics.total_time_ms << " ms\n";
        
        if (metrics.kernel_execution_time_ms > 0) {
            ss << "  Kernel Execution Time: " << metrics.kernel_execution_time_ms << " ms\n";
        }
        
        if (metrics.memory_transfer_time_ms > 0) {
            ss << "  Memory Transfer Time: " << metrics.memory_transfer_time_ms << " ms\n";
        }
        
        if (metrics.memory_used_bytes > 0) {
            ss << "  Memory Used: " << (metrics.memory_used_bytes / (1024.0 * 1024.0)) << " MB\n";
        }
        
        if (metrics.blocks_used > 0 && metrics.threads_per_block > 0) {
            ss << "  Grid Configuration: " << metrics.blocks_used << " blocks x " 
               << metrics.threads_per_block << " threads\n";
        }
        
        ss << "\n";
    }
    
    return ss.str();
}

bool MFPCUDA::initializeKernels() {
    // Method 1: Expanded q Factorization
    const char* method1_kernel_source = R"(
        extern "C" __global__ void method1_kernel(const char* number, size_t number_size, char* factors, size_t factors_size) {
            // TODO: Implement Method 1 kernel
        }
    )";
    
    method1_kernel_ = context_->createKernel("method1_kernel", method1_kernel_source);
    if (method1_kernel_->getStatus() != CUDAStatus::SUCCESS) {
        return false;
    }
    
    // Method 2: Ultrafast with Structural Filter
    const char* method2_kernel_source = R"(
        extern "C" __global__ void method2_kernel(const char* number, size_t number_size, char* factors, size_t factors_size) {
            // TODO: Implement Method 2 kernel
        }
    )";
    
    method2_kernel_ = context_->createKernel("method2_kernel", method2_kernel_source);
    if (method2_kernel_->getStatus() != CUDAStatus::SUCCESS) {
        return false;
    }
    
    // Method 3: Parallelized with Dynamic Blocks
    const char* method3_kernel_source = R"(
        extern "C" __global__ void method3_kernel(const char* number, size_t number_size, char* factors, size_t factors_size) {
            // TODO: Implement Method 3 kernel
        }
    )";
    
    method3_kernel_ = context_->createKernel("method3_kernel", method3_kernel_source);
    if (method3_kernel_->getStatus() != CUDAStatus::SUCCESS) {
        return false;
    }
    
    // Is Prime kernel
    const char* is_prime_kernel_source = R"(
        extern "C" __global__ void is_prime_kernel(const char* number, size_t number_size, int* result) {
            // TODO: Implement Is Prime kernel
        }
    )";
    
    is_prime_kernel_ = context_->createKernel("is_prime_kernel", is_prime_kernel_source);
    if (is_prime_kernel_->getStatus() != CUDAStatus::SUCCESS) {
        return false;
    }
    
    // Find Next Prime kernel
    const char* find_next_prime_kernel_source = R"(
        extern "C" __global__ void find_next_prime_kernel(const char* number, size_t number_size, char* result, size_t result_size) {
            // TODO: Implement Find Next Prime kernel
        }
    )";
    
    find_next_prime_kernel_ = context_->createKernel("find_next_prime_kernel", find_next_prime_kernel_source);
    if (find_next_prime_kernel_->getStatus() != CUDAStatus::SUCCESS) {
        return false;
    }
    
    return true;
}

void MFPCUDA::logPerformance(const PerformanceMetrics& metrics) {
    performance_metrics_.push_back(metrics);
}

bool MFPCUDA::implementMethod1(const std::string& number, std::vector<std::string>& factors) {
    if (!method1_kernel_) {
        return false;
    }
    
    // Allocate memory for input number
    size_t number_size = number.size();
    auto device_number = context_->allocateMemory(number_size + 1);
    if (device_number->getStatus() != CUDAStatus::SUCCESS) {
        return false;
    }
    
    // Copy number to device
    device_number->copyFromHost(number.c_str(), number_size + 1);
    
    // Allocate memory for factors
    // Allocate enough space for potential factors
    size_t factors_size = number_size * 10;
    auto device_factors = context_->allocateMemory(factors_size);
    if (device_factors->getStatus() != CUDAStatus::SUCCESS) {
        return false;
    }
    
    // Initialize factors memory
    device_factors->memset(0);
    
    // Set kernel arguments
    method1_kernel_->setArgs(device_number->getPtr(), number_size, device_factors->getPtr(), factors_size);
    
    // Determine grid and block dimensions
    const system::GPUInfo* device_info = context_->getDeviceInfo();
    int threads_per_block = 256;
    int blocks = (device_info->compute_info.cuda_cores + threads_per_block - 1) / threads_per_block;
    blocks = std::min(blocks, 65535); // Maximum grid dimension
    
    // Launch kernel
    dim3 grid_dim(blocks);
    dim3 block_dim(threads_per_block);
    
    auto kernel_start = std::chrono::high_resolution_clock::now();
    method1_kernel_->launch(grid_dim, block_dim);
    auto kernel_end = std::chrono::high_resolution_clock::now();
    
    // Copy factors back
    std::vector<char> factors_buffer(factors_size);
    
    auto transfer_start = std::chrono::high_resolution_clock::now();
    device_factors->copyToHost(factors_buffer.data(), factors_size);
    auto transfer_end = std::chrono::high_resolution_clock::now();
    
    // Parse factors
    factors.clear();
    const char* ptr = factors_buffer.data();
    while (*ptr) {
        factors.push_back(ptr);
        ptr += strlen(ptr) + 1;
    }
    
    if (performance_logging_enabled_) {
        PerformanceMetrics metrics;
        metrics.kernel_execution_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(kernel_end - kernel_start).count();
        metrics.memory_transfer_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(transfer_end - transfer_start).count();
        metrics.memory_used_bytes = number_size + 1 + factors_size;
        metrics.blocks_used = blocks;
        metrics.threads_per_block = threads_per_block;
        metrics.method_name = "Method1_ExpandedQFactorization_Detail";
        logPerformance(metrics);
    }
    
    return true;
}

bool MFPCUDA::implementMethod2(const std::string& number, std::vector<std::string>& factors) {
    if (!method2_kernel_) {
        return false;
    }
    
    // Allocate memory for input number
    size_t number_size = number.size();
    auto device_number = context_->allocateMemory(number_size + 1);
    if (device_number->getStatus() != CUDAStatus::SUCCESS) {
        return false;
    }
    
    // Copy number to device
    device_number->copyFromHost(number.c_str(), number_size + 1);
    
    // Allocate memory for factors
    // Allocate enough space for potential factors
    size_t factors_size = number_size * 10;
    auto device_factors = context_->allocateMemory(factors_size);
    if (device_factors->getStatus() != CUDAStatus::SUCCESS) {
        return false;
    }
    
    // Initialize factors memory
    device_factors->memset(0);
    
    // Set kernel arguments
    method2_kernel_->setArgs(device_number->getPtr(), number_size, device_factors->getPtr(), factors_size);
    
    // Determine grid and block dimensions
    const system::GPUInfo* device_info = context_->getDeviceInfo();
    int threads_per_block = 256;
    int blocks = (device_info->compute_info.cuda_cores + threads_per_block - 1) / threads_per_block;
    blocks = std::min(blocks, 65535); // Maximum grid dimension
    
    // Launch kernel
    dim3 grid_dim(blocks);
    dim3 block_dim(threads_per_block);
    
    auto kernel_start = std::chrono::high_resolution_clock::now();
    method2_kernel_->launch(grid_dim, block_dim);
    auto kernel_end = std::chrono::high_resolution_clock::now();
    
    // Copy factors back
    std::vector<char> factors_buffer(factors_size);
    
    auto transfer_start = std::chrono::high_resolution_clock::now();
    device_factors->copyToHost(factors_buffer.data(), factors_size);
    auto transfer_end = std::chrono::high_resolution_clock::now();
    
    // Parse factors
    factors.clear();
    const char* ptr = factors_buffer.data();
    while (*ptr) {
        factors.push_back(ptr);
        ptr += strlen(ptr) + 1;
    }
    
    if (performance_logging_enabled_) {
        PerformanceMetrics metrics;
        metrics.kernel_execution_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(kernel_end - kernel_start).count();
        metrics.memory_transfer_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(transfer_end - transfer_start).count();
        metrics.memory_used_bytes = number_size + 1 + factors_size;
        metrics.blocks_used = blocks;
        metrics.threads_per_block = threads_per_block;
        metrics.method_name = "Method2_UltrafastWithStructuralFilter_Detail";
        logPerformance(metrics);
    }
    
    return true;
}

bool MFPCUDA::implementMethod3(const std::string& number, std::vector<std::string>& factors) {
    if (!method3_kernel_) {
        return false;
    }
    
    // Allocate memory for input number
    size_t number_size = number.size();
    auto device_number = context_->allocateMemory(number_size + 1);
    if (device_number->getStatus() != CUDAStatus::SUCCESS) {
        return false;
    }
    
    // Copy number to device
    device_number->copyFromHost(number.c_str(), number_size + 1);
    
    // Allocate memory for factors
    // Allocate enough space for potential factors
    size_t factors_size = number_size * 10;
    auto device_factors = context_->allocateMemory(factors_size);
    if (device_factors->getStatus() != CUDAStatus::SUCCESS) {
        return false;
    }
    
    // Initialize factors memory
    device_factors->memset(0);
    
    // Set kernel arguments
    method3_kernel_->setArgs(device_number->getPtr(), number_size, device_factors->getPtr(), factors_size);
    
    // Determine grid and block dimensions
    const system::GPUInfo* device_info = context_->getDeviceInfo();
    int threads_per_block = 256;
    int blocks = (device_info->compute_info.cuda_cores + threads_per_block - 1) / threads_per_block;
    blocks = std::min(blocks, 65535); // Maximum grid dimension
    
    // Launch kernel
    dim3 grid_dim(blocks);
    dim3 block_dim(threads_per_block);
    
    auto kernel_start = std::chrono::high_resolution_clock::now();
    method3_kernel_->launch(grid_dim, block_dim);
    auto kernel_end = std::chrono::high_resolution_clock::now();
    
    // Copy factors back
    std::vector<char> factors_buffer(factors_size);
    
    auto transfer_start = std::chrono::high_resolution_clock::now();
    device_factors->copyToHost(factors_buffer.data(), factors_size);
    auto transfer_end = std::chrono::high_resolution_clock::now();
    
    // Parse factors
    factors.clear();
    const char* ptr = factors_buffer.data();
    while (*ptr) {
        factors.push_back(ptr);
        ptr += strlen(ptr) + 1;
    }
    
    if (performance_logging_enabled_) {
        PerformanceMetrics metrics;
        metrics.kernel_execution_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(kernel_end - kernel_start).count();
        metrics.memory_transfer_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(transfer_end - transfer_start).count();
        metrics.memory_used_bytes = number_size + 1 + factors_size;
        metrics.blocks_used = blocks;
        metrics.threads_per_block = threads_per_block;
        metrics.method_name = "Method3_ParallelizedWithDynamicBlocks_Detail";
        logPerformance(metrics);
    }
    
    return true;
}

} // namespace cuda
} // namespace mfp
