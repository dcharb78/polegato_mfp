#include "metal_accelerator.h"
#include <chrono>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <iostream>
#include <fstream>

// Metal includes (if available)
#if defined(__APPLE__) && defined(HAVE_METAL)
#include <Metal/Metal.hpp>
#include <Foundation/Foundation.hpp>
#endif

namespace mfp {
namespace metal {

//=============================================================================
// Metal Context Implementation
//=============================================================================

MetalContext::MetalContext() 
    : status_(MetalStatus::SUCCESS), 
      device_count_(0), 
      current_device_(-1), 
      metal_device_(nullptr),
      default_library_(nullptr) {
}

MetalContext::~MetalContext() {
#if defined(__APPLE__) && defined(HAVE_METAL)
    // Release Metal resources
    if (default_library_) {
        MTLLibrary* library = static_cast<MTLLibrary*>(default_library_);
        [library release];
        default_library_ = nullptr;
    }
    
    if (metal_device_) {
        MTLDevice* device = static_cast<MTLDevice*>(metal_device_);
        [device release];
        metal_device_ = nullptr;
    }
#endif
}

MetalStatus MetalContext::initialize(int device_id) {
#if defined(__APPLE__) && defined(HAVE_METAL)
    // Get all Metal devices
    NSArray<id<MTLDevice>>* devices = MTLCopyAllDevices();
    device_count_ = [devices count];
    
    if (device_count_ == 0) {
        status_ = MetalStatus::ERROR_INVALID_DEVICE;
        error_message_ = "No Metal-capable devices found";
        [devices release];
        return status_;
    }
    
    // Select device
    if (device_id < 0) {
        // Auto-select best device (prefer discrete GPU)
        int best_device = 0;
        bool found_discrete = false;
        
        for (int i = 0; i < device_count_; ++i) {
            id<MTLDevice> device = devices[i];
            
            // Check if device is discrete GPU
            if ([device isLowPower] == NO && !found_discrete) {
                best_device = i;
                found_discrete = true;
            }
            
            // If we already found a discrete GPU, prefer the one with more memory
            if ([device isLowPower] == NO && found_discrete) {
                id<MTLDevice> current_best = devices[best_device];
                if ([device recommendedMaxWorkingSetSize] > [current_best recommendedMaxWorkingSetSize]) {
                    best_device = i;
                }
            }
        }
        
        device_id = best_device;
    } else if (device_id >= device_count_) {
        status_ = MetalStatus::ERROR_INVALID_DEVICE;
        error_message_ = "Invalid device ID: " + std::to_string(device_id);
        [devices release];
        return status_;
    }
    
    // Get selected device
    id<MTLDevice> device = devices[device_id];
    [device retain]; // Retain the device
    metal_device_ = static_cast<void*>(device);
    current_device_ = device_id;
    
    // Get device properties
    status_ = getDeviceProperties(current_device_, device_info_);
    
    // Create default library
    NSError* error = nil;
    id<MTLLibrary> library = [device newDefaultLibrary];
    
    if (!library) {
        status_ = MetalStatus::ERROR_INITIALIZATION;
        error_message_ = "Failed to create default Metal library";
        if (error) {
            error_message_ += ": " + std::string([[error localizedDescription] UTF8String]);
        }
        [devices release];
        return status_;
    }
    
    default_library_ = static_cast<void*>(library);
    
    [devices release];
    return MetalStatus::SUCCESS;
#else
    status_ = MetalStatus::ERROR_NOT_IMPLEMENTED;
    error_message_ = "Metal support not compiled in";
    return status_;
#endif
}

const system::GPUInfo* MetalContext::getDeviceInfo() const {
    if (current_device_ < 0) {
        return nullptr;
    }
    return &device_info_;
}

MetalStatus MetalContext::getStatus() const {
    return status_;
}

std::string MetalContext::getErrorMessage() const {
    return error_message_;
}

bool MetalContext::isAvailable() const {
#if defined(__APPLE__) && defined(HAVE_METAL)
    return device_count_ > 0 && current_device_ >= 0 && metal_device_ != nullptr;
#else
    return false;
#endif
}

int MetalContext::getDeviceCount() const {
    return device_count_;
}

MetalStatus MetalContext::setDevice(int device_id) {
#if defined(__APPLE__) && defined(HAVE_METAL)
    if (device_id < 0 || device_id >= device_count_) {
        status_ = MetalStatus::ERROR_INVALID_DEVICE;
        error_message_ = "Invalid device ID: " + std::to_string(device_id);
        return status_;
    }
    
    // Release current device and library
    if (default_library_) {
        MTLLibrary* library = static_cast<MTLLibrary*>(default_library_);
        [library release];
        default_library_ = nullptr;
    }
    
    if (metal_device_) {
        MTLDevice* device = static_cast<MTLDevice*>(metal_device_);
        [device release];
        metal_device_ = nullptr;
    }
    
    // Get all Metal devices
    NSArray<id<MTLDevice>>* devices = MTLCopyAllDevices();
    
    // Get selected device
    id<MTLDevice> device = devices[device_id];
    [device retain]; // Retain the device
    metal_device_ = static_cast<void*>(device);
    current_device_ = device_id;
    
    // Get device properties
    status_ = getDeviceProperties(current_device_, device_info_);
    
    // Create default library
    NSError* error = nil;
    id<MTLLibrary> library = [device newDefaultLibrary];
    
    if (!library) {
        status_ = MetalStatus::ERROR_INITIALIZATION;
        error_message_ = "Failed to create default Metal library";
        if (error) {
            error_message_ += ": " + std::string([[error localizedDescription] UTF8String]);
        }
        [devices release];
        return status_;
    }
    
    default_library_ = static_cast<void*>(library);
    
    [devices release];
    return MetalStatus::SUCCESS;
#else
    status_ = MetalStatus::ERROR_NOT_IMPLEMENTED;
    error_message_ = "Metal support not compiled in";
    return status_;
#endif
}

int MetalContext::getCurrentDevice() const {
    return current_device_;
}

std::shared_ptr<MetalCommandQueue> MetalContext::createCommandQueue() {
    return std::make_shared<MetalCommandQueue>(this);
}

std::shared_ptr<MetalBuffer> MetalContext::createBuffer(size_t size_bytes, MetalBufferType buffer_type) {
    return std::make_shared<MetalBuffer>(this, size_bytes, buffer_type);
}

std::shared_ptr<MetalComputePipeline> MetalContext::createComputePipeline(const std::string& function_name, const std::string& shader_source) {
    auto pipeline = std::make_shared<MetalComputePipeline>(this, function_name);
    pipeline->compile(shader_source);
    return pipeline;
}

std::shared_ptr<MetalComputePipeline> MetalContext::loadComputePipeline(const std::string& function_name, const std::string& library_path) {
    auto pipeline = std::make_shared<MetalComputePipeline>(this, function_name);
    pipeline->load(library_path);
    return pipeline;
}

void* MetalContext::getNativeDevice() const {
    return metal_device_;
}

MetalStatus MetalContext::getDeviceCount(int& count) {
#if defined(__APPLE__) && defined(HAVE_METAL)
    NSArray<id<MTLDevice>>* devices = MTLCopyAllDevices();
    count = [devices count];
    [devices release];
    return MetalStatus::SUCCESS;
#else
    count = 0;
    status_ = MetalStatus::ERROR_NOT_IMPLEMENTED;
    error_message_ = "Metal support not compiled in";
    return status_;
#endif
}

MetalStatus MetalContext::getDeviceProperties(int device_id, system::GPUInfo& info) {
#if defined(__APPLE__) && defined(HAVE_METAL)
    NSArray<id<MTLDevice>>* devices = MTLCopyAllDevices();
    
    if (device_id < 0 || device_id >= [devices count]) {
        status_ = MetalStatus::ERROR_INVALID_DEVICE;
        error_message_ = "Invalid device ID: " + std::to_string(device_id);
        [devices release];
        return status_;
    }
    
    id<MTLDevice> device = devices[device_id];
    
    // Fill GPU info
    info.device_id = device_id;
    info.name = [[device name] UTF8String];
    info.vendor = system::GPUVendor::APPLE;
    
    // Determine architecture based on device type
    if ([device isLowPower]) {
        info.architecture = system::GPUArchitecture::APPLE_INTEGRATED;
    } else {
        info.architecture = system::GPUArchitecture::APPLE_DISCRETE;
    }
    
    // Add Metal API support
    info.api_support.push_back(system::GPUAPISupport::METAL);
    
    // Memory information
    info.memory_info.total_memory_bytes = [device recommendedMaxWorkingSetSize];
    
    // Check if device supports unified memory
    info.memory_info.has_unified_memory = [device hasUnifiedMemory];
    
    // Compute information
    if (@available(macOS 10.15, iOS 13.0, *)) {
        info.compute_info.max_threads_per_threadgroup = [device maxThreadsPerThreadgroup].width;
    } else {
        info.compute_info.max_threads_per_threadgroup = 1024; // Default value
    }
    
    // Check if integrated
    info.is_integrated = [device isLowPower];
    
    // Check if headless
    info.is_headless = [device isHeadless];
    
    // Check if removable
    if (@available(macOS 10.13, iOS 11.0, *)) {
        info.is_removable = [device isRemovable];
    } else {
        info.is_removable = false;
    }
    
    [devices release];
    return MetalStatus::SUCCESS;
#else
    status_ = MetalStatus::ERROR_NOT_IMPLEMENTED;
    error_message_ = "Metal support not compiled in";
    return status_;
#endif
}

//=============================================================================
// Metal Command Queue Implementation
//=============================================================================

MetalCommandQueue::MetalCommandQueue(MetalContext* context) 
    : context_(context), 
      status_(MetalStatus::SUCCESS), 
      command_queue_(nullptr) {
#if defined(__APPLE__) && defined(HAVE_METAL)
    if (!context_->metal_device_) {
        status_ = MetalStatus::ERROR_INITIALIZATION;
        error_message_ = "Metal device not initialized";
        return;
    }
    
    id<MTLDevice> device = static_cast<id<MTLDevice>>(context_->metal_device_);
    id<MTLCommandQueue> queue = [device newCommandQueue];
    
    if (!queue) {
        status_ = MetalStatus::ERROR_INITIALIZATION;
        error_message_ = "Failed to create Metal command queue";
        return;
    }
    
    command_queue_ = static_cast<void*>(queue);
#else
    status_ = MetalStatus::ERROR_NOT_IMPLEMENTED;
    error_message_ = "Metal support not compiled in";
#endif
}

MetalCommandQueue::~MetalCommandQueue() {
#if defined(__APPLE__) && defined(HAVE_METAL)
    if (command_queue_) {
        id<MTLCommandQueue> queue = static_cast<id<MTLCommandQueue>>(command_queue_);
        [queue release];
        command_queue_ = nullptr;
    }
#endif
}

void* MetalCommandQueue::createCommandBuffer() {
#if defined(__APPLE__) && defined(HAVE_METAL)
    if (!command_queue_) {
        status_ = MetalStatus::ERROR_INITIALIZATION;
        error_message_ = "Command queue not initialized";
        return nullptr;
    }
    
    id<MTLCommandQueue> queue = static_cast<id<MTLCommandQueue>>(command_queue_);
    id<MTLCommandBuffer> buffer = [queue commandBuffer];
    
    if (!buffer) {
        status_ = MetalStatus::ERROR_INITIALIZATION;
        error_message_ = "Failed to create Metal command buffer";
        return nullptr;
    }
    
    [buffer retain]; // Retain the buffer
    return static_cast<void*>(buffer);
#else
    status_ = MetalStatus::ERROR_NOT_IMPLEMENTED;
    error_message_ = "Metal support not compiled in";
    return nullptr;
#endif
}

MetalStatus MetalCommandQueue::getStatus() const {
    return status_;
}

std::string MetalCommandQueue::getErrorMessage() const {
    return error_message_;
}

void* MetalCommandQueue::getNativeCommandQueue() const {
    return command_queue_;
}

//=============================================================================
// Metal Buffer Implementation
//=============================================================================

MetalBuffer::MetalBuffer(MetalContext* context, size_t size_bytes, MetalBufferType buffer_type) 
    : context_(context), 
      status_(MetalStatus::SUCCESS), 
      buffer_(nullptr), 
      ptr_(nullptr), 
      size_bytes_(size_bytes), 
      buffer_type_(buffer_type) {
#if defined(__APPLE__) && defined(HAVE_METAL)
    if (!context_->metal_device_) {
        status_ = MetalStatus::ERROR_INITIALIZATION;
        error_message_ = "Metal device not initialized";
        return;
    }
    
    id<MTLDevice> device = static_cast<id<MTLDevice>>(context_->metal_device_);
    MTLResourceOptions options;
    
    switch (buffer_type) {
        case MetalBufferType::SHARED:
            options = MTLResourceStorageModeShared;
            break;
            
        case MetalBufferType::MANAGED:
            options = MTLResourceStorageModeManaged;
            break;
            
        case MetalBufferType::PRIVATE:
            options = MTLResourceStorageModePrivate;
            break;
    }
    
    id<MTLBuffer> buffer = [device newBufferWithLength:size_bytes options:options];
    
    if (!buffer) {
        status_ = MetalStatus::ERROR_MEMORY_ALLOCATION;
        error_message_ = "Failed to create Metal buffer";
        return;
    }
    
    buffer_ = static_cast<void*>(buffer);
    
    // Get direct pointer for shared buffers
    if (buffer_type == MetalBufferType::SHARED || buffer_type == MetalBufferType::MANAGED) {
        ptr_ = [buffer contents];
    }
#else
    status_ = MetalStatus::ERROR_NOT_IMPLEMENTED;
    error_message_ = "Metal support not compiled in";
#endif
}

MetalBuffer::~MetalBuffer() {
#if defined(__APPLE__) && defined(HAVE_METAL)
    if (buffer_) {
        id<MTLBuffer> buffer = static_cast<id<MTLBuffer>>(buffer_);
        [buffer release];
        buffer_ = nullptr;
        ptr_ = nullptr;
    }
#endif
}

MetalStatus MetalBuffer::copyFromHost(const void* host_ptr, size_t size_bytes, size_t offset) {
#if defined(__APPLE__) && defined(HAVE_METAL)
    if (!buffer_) {
        status_ = MetalStatus::ERROR_MEMORY_ALLOCATION;
        error_message_ = "Buffer not allocated";
        return status_;
    }
    
    if (!host_ptr) {
        status_ = MetalStatus::ERROR_MEMORY_COPY;
        error_message_ = "Invalid host pointer";
        return status_;
    }
    
    if (size_bytes == 0) {
        size_bytes = size_bytes_ - offset;
    }
    
    if (offset + size_bytes > size_bytes_) {
        status_ = MetalStatus::ERROR_MEMORY_COPY;
        error_message_ = "Copy size exceeds allocated memory";
        return status_;
    }
    
    if (buffer_type_ == MetalBufferType::SHARED || buffer_type_ == MetalBufferType::MANAGED) {
        // Direct copy for shared or managed buffers
        memcpy(static_cast<char*>(ptr_) + offset, host_ptr, size_bytes);
        
        // Synchronize managed buffers
        if (buffer_type_ == MetalBufferType::MANAGED) {
            id<MTLBuffer> buffer = static_cast<id<MTLBuffer>>(buffer_);
            NSRange range = NSMakeRange(offset, size_bytes);
            [buffer didModifyRange:range];
        }
    } else {
        // For private buffers, we need to create a temporary shared buffer and use a blit command
        auto temp_buffer = context_->createBuffer(size_bytes, MetalBufferType::SHARED);
        if (temp_buffer->getStatus() != MetalStatus::SUCCESS) {
            status_ = temp_buffer->getStatus();
            error_message_ = "Failed to create temporary buffer: " + temp_buffer->getErrorMessage();
            return status_;
        }
        
        // Copy to temporary buffer
        memcpy(temp_buffer->getPtr(), host_ptr, size_bytes);
        
        // Copy from temporary buffer to this buffer
        auto command_queue = context_->createCommandQueue();
        if (command_queue->getStatus() != MetalStatus::SUCCESS) {
            status_ = command_queue->getStatus();
            error_message_ = "Failed to create command queue: " + command_queue->getErrorMessage();
            return status_;
        }
        
        id<MTLCommandBuffer> command_buffer = static_cast<id<MTLCommandBuffer>>(command_queue->createCommandBuffer());
        if (!command_buffer) {
            status_ = MetalStatus::ERROR_INITIALIZATION;
            error_message_ = "Failed to create command buffer";
            return status_;
        }
        
        id<MTLBlitCommandEncoder> blit_encoder = [command_buffer blitCommandEncoder];
        if (!blit_encoder) {
            status_ = MetalStatus::ERROR_INITIALIZATION;
            error_message_ = "Failed to create blit command encoder";
            [command_buffer release];
            return status_;
        }
        
        id<MTLBuffer> src_buffer = static_cast<id<MTLBuffer>>(temp_buffer->buffer_);
        id<MTLBuffer> dst_buffer = static_cast<id<MTLBuffer>>(buffer_);
        
        [blit_encoder copyFromBuffer:src_buffer
                        sourceOffset:0
                            toBuffer:dst_buffer
                   destinationOffset:offset
                                size:size_bytes];
        
        [blit_encoder endEncoding];
        [command_buffer commit];
        [command_buffer waitUntilCompleted];
        [command_buffer release];
    }
    
    return MetalStatus::SUCCESS;
#else
    status_ = MetalStatus::ERROR_NOT_IMPLEMENTED;
    error_message_ = "Metal support not compiled in";
    return status_;
#endif
}

MetalStatus MetalBuffer::copyToHost(void* host_ptr, size_t size_bytes, size_t offset) {
#if defined(__APPLE__) && defined(HAVE_METAL)
    if (!buffer_) {
        status_ = MetalStatus::ERROR_MEMORY_ALLOCATION;
        error_message_ = "Buffer not allocated";
        return status_;
    }
    
    if (!host_ptr) {
        status_ = MetalStatus::ERROR_MEMORY_COPY;
        error_message_ = "Invalid host pointer";
        return status_;
    }
    
    if (size_bytes == 0) {
        size_bytes = size_bytes_ - offset;
    }
    
    if (offset + size_bytes > size_bytes_) {
        status_ = MetalStatus::ERROR_MEMORY_COPY;
        error_message_ = "Copy size exceeds allocated memory";
        return status_;
    }
    
    if (buffer_type_ == MetalBufferType::SHARED || buffer_type_ == MetalBufferType::MANAGED) {
        // Direct copy for shared or managed buffers
        memcpy(host_ptr, static_cast<char*>(ptr_) + offset, size_bytes);
    } else {
        // For private buffers, we need to create a temporary shared buffer and use a blit command
        auto temp_buffer = context_->createBuffer(size_bytes, MetalBufferType::SHARED);
        if (temp_buffer->getStatus() != MetalStatus::SUCCESS) {
            status_ = temp_buffer->getStatus();
            error_message_ = "Failed to create temporary buffer: " + temp_buffer->getErrorMessage();
            return status_;
        }
        
        // Copy from this buffer to temporary buffer
        auto command_queue = context_->createCommandQueue();
        if (command_queue->getStatus() != MetalStatus::SUCCESS) {
            status_ = command_queue->getStatus();
            error_message_ = "Failed to create command queue: " + command_queue->getErrorMessage();
            return status_;
        }
        
        id<MTLCommandBuffer> command_buffer = static_cast<id<MTLCommandBuffer>>(command_queue->createCommandBuffer());
        if (!command_buffer) {
            status_ = MetalStatus::ERROR_INITIALIZATION;
            error_message_ = "Failed to create command buffer";
            return status_;
        }
        
        id<MTLBlitCommandEncoder> blit_encoder = [command_buffer blitCommandEncoder];
        if (!blit_encoder) {
            status_ = MetalStatus::ERROR_INITIALIZATION;
            error_message_ = "Failed to create blit command encoder";
            [command_buffer release];
            return status_;
        }
        
        id<MTLBuffer> src_buffer = static_cast<id<MTLBuffer>>(buffer_);
        id<MTLBuffer> dst_buffer = static_cast<id<MTLBuffer>>(temp_buffer->buffer_);
        
        [blit_encoder copyFromBuffer:src_buffer
                        sourceOffset:offset
                            toBuffer:dst_buffer
                   destinationOffset:0
                                size:size_bytes];
        
        [blit_encoder endEncoding];
        [command_buffer commit];
        [command_buffer waitUntilCompleted];
        [command_buffer release];
        
        // Copy from temporary buffer to host
        memcpy(host_ptr, temp_buffer->getPtr(), size_bytes);
    }
    
    return MetalStatus::SUCCESS;
#else
    status_ = MetalStatus::ERROR_NOT_IMPLEMENTED;
    error_message_ = "Metal support not compiled in";
    return status_;
#endif
}

MetalStatus MetalBuffer::copyFromBuffer(const MetalBuffer& source, size_t size_bytes, size_t dest_offset, size_t source_offset) {
#if defined(__APPLE__) && defined(HAVE_METAL)
    if (!buffer_) {
        status_ = MetalStatus::ERROR_MEMORY_ALLOCATION;
        error_message_ = "Destination buffer not allocated";
        return status_;
    }
    
    if (!source.buffer_) {
        status_ = MetalStatus::ERROR_MEMORY_ALLOCATION;
        error_message_ = "Source buffer not allocated";
        return status_;
    }
    
    if (size_bytes == 0) {
        size_bytes = std::min(size_bytes_ - dest_offset, source.size_bytes_ - source_offset);
    }
    
    if (dest_offset + size_bytes > size_bytes_) {
        status_ = MetalStatus::ERROR_MEMORY_COPY;
        error_message_ = "Copy size exceeds destination allocated memory";
        return status_;
    }
    
    if (source_offset + size_bytes > source.size_bytes_) {
        status_ = MetalStatus::ERROR_MEMORY_COPY;
        error_message_ = "Copy size exceeds source allocated memory";
        return status_;
    }
    
    // Create command queue and buffer
    auto command_queue = context_->createCommandQueue();
    if (command_queue->getStatus() != MetalStatus::SUCCESS) {
        status_ = command_queue->getStatus();
        error_message_ = "Failed to create command queue: " + command_queue->getErrorMessage();
        return status_;
    }
    
    id<MTLCommandBuffer> command_buffer = static_cast<id<MTLCommandBuffer>>(command_queue->createCommandBuffer());
    if (!command_buffer) {
        status_ = MetalStatus::ERROR_INITIALIZATION;
        error_message_ = "Failed to create command buffer";
        return status_;
    }
    
    // Create blit encoder
    id<MTLBlitCommandEncoder> blit_encoder = [command_buffer blitCommandEncoder];
    if (!blit_encoder) {
        status_ = MetalStatus::ERROR_INITIALIZATION;
        error_message_ = "Failed to create blit command encoder";
        [command_buffer release];
        return status_;
    }
    
    // Copy buffer
    id<MTLBuffer> src_buffer = static_cast<id<MTLBuffer>>(source.buffer_);
    id<MTLBuffer> dst_buffer = static_cast<id<MTLBuffer>>(buffer_);
    
    [blit_encoder copyFromBuffer:src_buffer
                    sourceOffset:source_offset
                        toBuffer:dst_buffer
               destinationOffset:dest_offset
                            size:size_bytes];
    
    [blit_encoder endEncoding];
    [command_buffer commit];
    [command_buffer waitUntilCompleted];
    [command_buffer release];
    
    return MetalStatus::SUCCESS;
#else
    status_ = MetalStatus::ERROR_NOT_IMPLEMENTED;
    error_message_ = "Metal support not compiled in";
    return status_;
#endif
}

void* MetalBuffer::getPtr() const {
    return ptr_;
}

size_t MetalBuffer::getSize() const {
    return size_bytes_;
}

MetalBufferType MetalBuffer::getBufferType() const {
    return buffer_type_;
}

MetalStatus MetalBuffer::getStatus() const {
    return status_;
}

std::string MetalBuffer::getErrorMessage() const {
    return error_message_;
}

void* MetalBuffer::getNativeBuffer() const {
    return buffer_;
}

//=============================================================================
// Metal Compute Pipeline Implementation
//=============================================================================

MetalComputePipeline::MetalComputePipeline(MetalContext* context, const std::string& function_name) 
    : context_(context), 
      status_(MetalStatus::SUCCESS), 
      function_name_(function_name), 
      compute_pipeline_state_(nullptr), 
      compute_function_(nullptr),
      thread_group_size_x_(0),
      thread_group_size_y_(0),
      thread_group_size_z_(0) {
}

MetalComputePipeline::~MetalComputePipeline() {
#if defined(__APPLE__) && defined(HAVE_METAL)
    if (compute_function_) {
        id<MTLFunction> function = static_cast<id<MTLFunction>>(compute_function_);
        [function release];
        compute_function_ = nullptr;
    }
    
    if (compute_pipeline_state_) {
        id<MTLComputePipelineState> pipeline_state = static_cast<id<MTLComputePipelineState>>(compute_pipeline_state_);
        [pipeline_state release];
        compute_pipeline_state_ = nullptr;
    }
#endif
}

MetalStatus MetalComputePipeline::compile(const std::string& shader_source) {
#if defined(__APPLE__) && defined(HAVE_METAL)
    if (!context_->metal_device_) {
        status_ = MetalStatus::ERROR_INITIALIZATION;
        error_message_ = "Metal device not initialized";
        return status_;
    }
    
    // Create Metal library from source
    id<MTLDevice> device = static_cast<id<MTLDevice>>(context_->metal_device_);
    NSError* error = nil;
    
    NSString* source_str = [NSString stringWithUTF8String:shader_source.c_str()];
    MTLCompileOptions* options = [MTLCompileOptions new];
    
    id<MTLLibrary> library = [device newLibraryWithSource:source_str options:options error:&error];
    [options release];
    
    if (!library) {
        status_ = MetalStatus::ERROR_SHADER_COMPILATION;
        error_message_ = "Failed to compile Metal shader";
        if (error) {
            error_message_ += ": " + std::string([[error localizedDescription] UTF8String]);
        }
        return status_;
    }
    
    // Get function
    NSString* function_name_str = [NSString stringWithUTF8String:function_name_.c_str()];
    id<MTLFunction> function = [library newFunctionWithName:function_name_str];
    
    if (!function) {
        status_ = MetalStatus::ERROR_SHADER_COMPILATION;
        error_message_ = "Failed to get Metal function: " + function_name_;
        [library release];
        return status_;
    }
    
    compute_function_ = static_cast<void*>(function);
    
    // Create compute pipeline state
    status_ = createComputePipelineState();
    
    [library release];
    return status_;
#else
    status_ = MetalStatus::ERROR_NOT_IMPLEMENTED;
    error_message_ = "Metal support not compiled in";
    return status_;
#endif
}

MetalStatus MetalComputePipeline::load(const std::string& library_path) {
#if defined(__APPLE__) && defined(HAVE_METAL)
    if (!context_->metal_device_) {
        status_ = MetalStatus::ERROR_INITIALIZATION;
        error_message_ = "Metal device not initialized";
        return status_;
    }
    
    // Load Metal library from file
    id<MTLDevice> device = static_cast<id<MTLDevice>>(context_->metal_device_);
    NSError* error = nil;
    
    NSURL* library_url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:library_path.c_str()]];
    id<MTLLibrary> library = [device newLibraryWithURL:library_url error:&error];
    
    if (!library) {
        status_ = MetalStatus::ERROR_SHADER_COMPILATION;
        error_message_ = "Failed to load Metal library: " + library_path;
        if (error) {
            error_message_ += ": " + std::string([[error localizedDescription] UTF8String]);
        }
        return status_;
    }
    
    // Get function
    NSString* function_name_str = [NSString stringWithUTF8String:function_name_.c_str()];
    id<MTLFunction> function = [library newFunctionWithName:function_name_str];
    
    if (!function) {
        status_ = MetalStatus::ERROR_SHADER_COMPILATION;
        error_message_ = "Failed to get Metal function: " + function_name_;
        [library release];
        return status_;
    }
    
    compute_function_ = static_cast<void*>(function);
    
    // Create compute pipeline state
    status_ = createComputePipelineState();
    
    [library release];
    return status_;
#else
    status_ = MetalStatus::ERROR_NOT_IMPLEMENTED;
    error_message_ = "Metal support not compiled in";
    return status_;
#endif
}

MetalStatus MetalComputePipeline::dispatch(MetalCommandQueue& queue, size_t grid_width, size_t grid_height, size_t grid_depth) {
#if defined(__APPLE__) && defined(HAVE_METAL)
    if (!compute_pipeline_state_) {
        status_ = MetalStatus::ERROR_INITIALIZATION;
        error_message_ = "Compute pipeline state not initialized";
        return status_;
    }
    
    if (!queue.command_queue_) {
        status_ = MetalStatus::ERROR_INITIALIZATION;
        error_message_ = "Command queue not initialized";
        return status_;
    }
    
    // Create command buffer
    id<MTLCommandBuffer> command_buffer = static_cast<id<MTLCommandBuffer>>(queue.createCommandBuffer());
    if (!command_buffer) {
        status_ = MetalStatus::ERROR_INITIALIZATION;
        error_message_ = "Failed to create command buffer";
        return status_;
    }
    
    // Create compute command encoder
    id<MTLComputeCommandEncoder> compute_encoder = [command_buffer computeCommandEncoder];
    if (!compute_encoder) {
        status_ = MetalStatus::ERROR_INITIALIZATION;
        error_message_ = "Failed to create compute command encoder";
        [command_buffer release];
        return status_;
    }
    
    // Set compute pipeline state
    id<MTLComputePipelineState> pipeline_state = static_cast<id<MTLComputePipelineState>>(compute_pipeline_state_);
    [compute_encoder setComputePipelineState:pipeline_state];
    
    // Calculate thread group size
    MTLSize thread_group_size = MTLSizeMake(thread_group_size_x_, thread_group_size_y_, thread_group_size_z_);
    MTLSize grid_size = MTLSizeMake(grid_width, grid_height, grid_depth);
    
    // Dispatch threads
    [compute_encoder dispatchThreads:grid_size threadsPerThreadgroup:thread_group_size];
    
    // End encoding and commit
    [compute_encoder endEncoding];
    [command_buffer commit];
    [command_buffer waitUntilCompleted];
    [command_buffer release];
    
    return MetalStatus::SUCCESS;
#else
    status_ = MetalStatus::ERROR_NOT_IMPLEMENTED;
    error_message_ = "Metal support not compiled in";
    return status_;
#endif
}

MetalStatus MetalComputePipeline::setBuffer(const MetalBuffer& buffer, size_t index) {
#if defined(__APPLE__) && defined(HAVE_METAL)
    if (!compute_pipeline_state_) {
        status_ = MetalStatus::ERROR_INITIALIZATION;
        error_message_ = "Compute pipeline state not initialized";
        return status_;
    }
    
    if (!buffer.buffer_) {
        status_ = MetalStatus::ERROR_MEMORY_ALLOCATION;
        error_message_ = "Buffer not allocated";
        return status_;
    }
    
    // Create command buffer
    auto command_queue = context_->createCommandQueue();
    if (command_queue->getStatus() != MetalStatus::SUCCESS) {
        status_ = command_queue->getStatus();
        error_message_ = "Failed to create command queue: " + command_queue->getErrorMessage();
        return status_;
    }
    
    id<MTLCommandBuffer> command_buffer = static_cast<id<MTLCommandBuffer>>(command_queue->createCommandBuffer());
    if (!command_buffer) {
        status_ = MetalStatus::ERROR_INITIALIZATION;
        error_message_ = "Failed to create command buffer";
        return status_;
    }
    
    // Create compute command encoder
    id<MTLComputeCommandEncoder> compute_encoder = [command_buffer computeCommandEncoder];
    if (!compute_encoder) {
        status_ = MetalStatus::ERROR_INITIALIZATION;
        error_message_ = "Failed to create compute command encoder";
        [command_buffer release];
        return status_;
    }
    
    // Set buffer
    id<MTLBuffer> metal_buffer = static_cast<id<MTLBuffer>>(buffer.buffer_);
    [compute_encoder setBuffer:metal_buffer offset:0 atIndex:index];
    
    // End encoding and commit
    [compute_encoder endEncoding];
    [command_buffer commit];
    [command_buffer waitUntilCompleted];
    [command_buffer release];
    
    return MetalStatus::SUCCESS;
#else
    status_ = MetalStatus::ERROR_NOT_IMPLEMENTED;
    error_message_ = "Metal support not compiled in";
    return status_;
#endif
}

template<typename T>
MetalStatus MetalComputePipeline::setBytes(const T& data, size_t index) {
#if defined(__APPLE__) && defined(HAVE_METAL)
    if (!compute_pipeline_state_) {
        status_ = MetalStatus::ERROR_INITIALIZATION;
        error_message_ = "Compute pipeline state not initialized";
        return status_;
    }
    
    // Create command buffer
    auto command_queue = context_->createCommandQueue();
    if (command_queue->getStatus() != MetalStatus::SUCCESS) {
        status_ = command_queue->getStatus();
        error_message_ = "Failed to create command queue: " + command_queue->getErrorMessage();
        return status_;
    }
    
    id<MTLCommandBuffer> command_buffer = static_cast<id<MTLCommandBuffer>>(command_queue->createCommandBuffer());
    if (!command_buffer) {
        status_ = MetalStatus::ERROR_INITIALIZATION;
        error_message_ = "Failed to create command buffer";
        return status_;
    }
    
    // Create compute command encoder
    id<MTLComputeCommandEncoder> compute_encoder = [command_buffer computeCommandEncoder];
    if (!compute_encoder) {
        status_ = MetalStatus::ERROR_INITIALIZATION;
        error_message_ = "Failed to create compute command encoder";
        [command_buffer release];
        return status_;
    }
    
    // Set bytes
    [compute_encoder setBytes:&data length:sizeof(T) atIndex:index];
    
    // End encoding and commit
    [compute_encoder endEncoding];
    [command_buffer commit];
    [command_buffer waitUntilCompleted];
    [command_buffer release];
    
    return MetalStatus::SUCCESS;
#else
    status_ = MetalStatus::ERROR_NOT_IMPLEMENTED;
    error_message_ = "Metal support not compiled in";
    return status_;
#endif
}

MetalStatus MetalComputePipeline::getStatus() const {
    return status_;
}

std::string MetalComputePipeline::getErrorMessage() const {
    return error_message_;
}

MetalStatus MetalComputePipeline::createComputePipelineState() {
#if defined(__APPLE__) && defined(HAVE_METAL)
    if (!context_->metal_device_) {
        status_ = MetalStatus::ERROR_INITIALIZATION;
        error_message_ = "Metal device not initialized";
        return status_;
    }
    
    if (!compute_function_) {
        status_ = MetalStatus::ERROR_INITIALIZATION;
        error_message_ = "Compute function not initialized";
        return status_;
    }
    
    // Create compute pipeline state
    id<MTLDevice> device = static_cast<id<MTLDevice>>(context_->metal_device_);
    id<MTLFunction> function = static_cast<id<MTLFunction>>(compute_function_);
    NSError* error = nil;
    
    id<MTLComputePipelineState> pipeline_state = [device newComputePipelineStateWithFunction:function error:&error];
    
    if (!pipeline_state) {
        status_ = MetalStatus::ERROR_INITIALIZATION;
        error_message_ = "Failed to create compute pipeline state";
        if (error) {
            error_message_ += ": " + std::string([[error localizedDescription] UTF8String]);
        }
        return status_;
    }
    
    compute_pipeline_state_ = static_cast<void*>(pipeline_state);
    
    // Get thread group size
    thread_group_size_x_ = [pipeline_state maxTotalThreadsPerThreadgroup];
    thread_group_size_y_ = 1;
    thread_group_size_z_ = 1;
    
    // Adjust thread group size for 2D and 3D grids
    if (thread_group_size_x_ > 32) {
        thread_group_size_x_ = 32;
        thread_group_size_y_ = 32;
    }
    
    return MetalStatus::SUCCESS;
#else
    status_ = MetalStatus::ERROR_NOT_IMPLEMENTED;
    error_message_ = "Metal support not compiled in";
    return status_;
#endif
}

//=============================================================================
// MFP Metal Implementation
//=============================================================================

MFPMetal::MFPMetal() 
    : performance_logging_enabled_(false) {
    context_ = std::make_unique<MetalContext>();
}

MFPMetal::~MFPMetal() {
    // Clean up resources
}

bool MFPMetal::initialize(int device_id) {
    // Initialize Metal context
    MetalStatus status = context_->initialize(device_id);
    if (status != MetalStatus::SUCCESS) {
        return false;
    }
    
    // Initialize shaders
    return initializeShaders();
}

bool MFPMetal::isAvailable() const {
    return context_->isAvailable();
}

MetalContext* MFPMetal::getContext() const {
    return context_.get();
}

const system::GPUInfo* MFPMetal::getDeviceInfo() const {
    return context_->getDeviceInfo();
}

bool MFPMetal::runMethod1(const std::string& number, std::vector<std::string>& factors) {
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

bool MFPMetal::runMethod2(const std::string& number, std::vector<std::string>& factors) {
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

bool MFPMetal::runMethod3(const std::string& number, std::vector<std::string>& factors) {
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

bool MFPMetal::isPrime(const std::string& number) {
    if (!is_prime_pipeline_) {
        return false;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Allocate buffer for input number
    size_t number_size = number.size();
    auto number_buffer = context_->createBuffer(number_size + 1);
    if (number_buffer->getStatus() != MetalStatus::SUCCESS) {
        return false;
    }
    
    // Copy number to buffer
    number_buffer->copyFromHost(number.c_str(), number_size + 1);
    
    // Allocate buffer for result
    auto result_buffer = context_->createBuffer(sizeof(int));
    if (result_buffer->getStatus() != MetalStatus::SUCCESS) {
        return false;
    }
    
    // Set result to 0 (not prime)
    int result_value = 0;
    result_buffer->copyFromHost(&result_value, sizeof(int));
    
    // Set buffers
    is_prime_pipeline_->setBuffer(*number_buffer, 0);
    is_prime_pipeline_->setBuffer(*result_buffer, 1);
    
    // Set number size
    is_prime_pipeline_->setBytes(number_size, 2);
    
    // Create command queue
    auto command_queue = context_->createCommandQueue();
    if (command_queue->getStatus() != MetalStatus::SUCCESS) {
        return false;
    }
    
    // Dispatch compute
    size_t grid_width = 1024;
    
    auto kernel_start = std::chrono::high_resolution_clock::now();
    is_prime_pipeline_->dispatch(*command_queue, grid_width);
    auto kernel_end = std::chrono::high_resolution_clock::now();
    
    // Copy result back
    auto transfer_start = std::chrono::high_resolution_clock::now();
    result_buffer->copyToHost(&result_value, sizeof(int));
    auto transfer_end = std::chrono::high_resolution_clock::now();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    
    if (performance_logging_enabled_) {
        PerformanceMetrics metrics;
        metrics.kernel_execution_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(kernel_end - kernel_start).count();
        metrics.memory_transfer_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(transfer_end - transfer_start).count();
        metrics.total_time_ms = duration;
        metrics.memory_used_bytes = number_size + 1 + sizeof(int);
        metrics.grid_width = grid_width;
        metrics.grid_height = 1;
        metrics.grid_depth = 1;
        metrics.method_name = "IsPrime";
        logPerformance(metrics);
    }
    
    return result_value != 0;
}

std::string MFPMetal::findNextPrime(const std::string& number) {
    if (!find_next_prime_pipeline_) {
        return "";
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Allocate buffer for input number
    size_t number_size = number.size();
    auto number_buffer = context_->createBuffer(number_size + 1);
    if (number_buffer->getStatus() != MetalStatus::SUCCESS) {
        return "";
    }
    
    // Copy number to buffer
    number_buffer->copyFromHost(number.c_str(), number_size + 1);
    
    // Allocate buffer for result (next prime)
    // Allocate extra space for potential growth
    size_t result_size = number_size + 100;
    auto result_buffer = context_->createBuffer(result_size);
    if (result_buffer->getStatus() != MetalStatus::SUCCESS) {
        return "";
    }
    
    // Set buffers
    find_next_prime_pipeline_->setBuffer(*number_buffer, 0);
    find_next_prime_pipeline_->setBuffer(*result_buffer, 1);
    
    // Set sizes
    find_next_prime_pipeline_->setBytes(number_size, 2);
    find_next_prime_pipeline_->setBytes(result_size, 3);
    
    // Create command queue
    auto command_queue = context_->createCommandQueue();
    if (command_queue->getStatus() != MetalStatus::SUCCESS) {
        return "";
    }
    
    // Dispatch compute
    size_t grid_width = 1024;
    
    auto kernel_start = std::chrono::high_resolution_clock::now();
    find_next_prime_pipeline_->dispatch(*command_queue, grid_width);
    auto kernel_end = std::chrono::high_resolution_clock::now();
    
    // Copy result back
    std::vector<char> result_buffer_data(result_size);
    
    auto transfer_start = std::chrono::high_resolution_clock::now();
    result_buffer->copyToHost(result_buffer_data.data(), result_size);
    auto transfer_end = std::chrono::high_resolution_clock::now();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    
    if (performance_logging_enabled_) {
        PerformanceMetrics metrics;
        metrics.kernel_execution_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(kernel_end - kernel_start).count();
        metrics.memory_transfer_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(transfer_end - transfer_start).count();
        metrics.total_time_ms = duration;
        metrics.memory_used_bytes = number_size + 1 + result_size;
        metrics.grid_width = grid_width;
        metrics.grid_height = 1;
        metrics.grid_depth = 1;
        metrics.method_name = "FindNextPrime";
        logPerformance(metrics);
    }
    
    return std::string(result_buffer_data.data());
}

bool MFPMetal::findPrimeFactors(const std::string& number, std::vector<std::string>& factors) {
    // Use Method 3 (Parallelized with Dynamic Blocks) by default
    return runMethod3(number, factors);
}

void MFPMetal::setPerformanceLogging(bool enable) {
    performance_logging_enabled_ = enable;
    
    if (!enable) {
        // Clear performance metrics
        performance_metrics_.clear();
    }
}

std::string MFPMetal::getPerformanceMetrics() const {
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
        
        if (metrics.grid_width > 0) {
            ss << "  Grid Configuration: " << metrics.grid_width;
            if (metrics.grid_height > 1 || metrics.grid_depth > 1) {
                ss << " x " << metrics.grid_height;
                if (metrics.grid_depth > 1) {
                    ss << " x " << metrics.grid_depth;
                }
            }
            ss << " threads\n";
        }
        
        ss << "\n";
    }
    
    return ss.str();
}

bool MFPMetal::initializeShaders() {
    // Method 1: Expanded q Factorization
    const char* method1_shader_source = R"(
        #include <metal_stdlib>
        using namespace metal;
        
        kernel void method1_kernel(device const char* number [[buffer(0)]],
                                  device char* factors [[buffer(1)]],
                                  device const uint& number_size [[buffer(2)]],
                                  device const uint& factors_size [[buffer(3)]],
                                  uint id [[thread_position_in_grid]]) {
            // TODO: Implement Method 1 kernel
        }
    )";
    
    method1_pipeline_ = context_->createComputePipeline("method1_kernel", method1_shader_source);
    if (method1_pipeline_->getStatus() != MetalStatus::SUCCESS) {
        return false;
    }
    
    // Method 2: Ultrafast with Structural Filter
    const char* method2_shader_source = R"(
        #include <metal_stdlib>
        using namespace metal;
        
        kernel void method2_kernel(device const char* number [[buffer(0)]],
                                  device char* factors [[buffer(1)]],
                                  device const uint& number_size [[buffer(2)]],
                                  device const uint& factors_size [[buffer(3)]],
                                  uint id [[thread_position_in_grid]]) {
            // TODO: Implement Method 2 kernel
        }
    )";
    
    method2_pipeline_ = context_->createComputePipeline("method2_kernel", method2_shader_source);
    if (method2_pipeline_->getStatus() != MetalStatus::SUCCESS) {
        return false;
    }
    
    // Method 3: Parallelized with Dynamic Blocks
    const char* method3_shader_source = R"(
        #include <metal_stdlib>
        using namespace metal;
        
        kernel void method3_kernel(device const char* number [[buffer(0)]],
                                  device char* factors [[buffer(1)]],
                                  device const uint& number_size [[buffer(2)]],
                                  device const uint& factors_size [[buffer(3)]],
                                  uint id [[thread_position_in_grid]]) {
            // TODO: Implement Method 3 kernel
        }
    )";
    
    method3_pipeline_ = context_->createComputePipeline("method3_kernel", method3_shader_source);
    if (method3_pipeline_->getStatus() != MetalStatus::SUCCESS) {
        return false;
    }
    
    // Is Prime kernel
    const char* is_prime_shader_source = R"(
        #include <metal_stdlib>
        using namespace metal;
        
        kernel void is_prime_kernel(device const char* number [[buffer(0)]],
                                   device int* result [[buffer(1)]],
                                   device const uint& number_size [[buffer(2)]],
                                   uint id [[thread_position_in_grid]]) {
            // TODO: Implement Is Prime kernel
        }
    )";
    
    is_prime_pipeline_ = context_->createComputePipeline("is_prime_kernel", is_prime_shader_source);
    if (is_prime_pipeline_->getStatus() != MetalStatus::SUCCESS) {
        return false;
    }
    
    // Find Next Prime kernel
    const char* find_next_prime_shader_source = R"(
        #include <metal_stdlib>
        using namespace metal;
        
        kernel void find_next_prime_kernel(device const char* number [[buffer(0)]],
                                          device char* result [[buffer(1)]],
                                          device const uint& number_size [[buffer(2)]],
                                          device const uint& result_size [[buffer(3)]],
                                          uint id [[thread_position_in_grid]]) {
            // TODO: Implement Find Next Prime kernel
        }
    )";
    
    find_next_prime_pipeline_ = context_->createComputePipeline("find_next_prime_kernel", find_next_prime_shader_source);
    if (find_next_prime_pipeline_->getStatus() != MetalStatus::SUCCESS) {
        return false;
    }
    
    return true;
}

void MFPMetal::logPerformance(const PerformanceMetrics& metrics) {
    performance_metrics_.push_back(metrics);
}

bool MFPMetal::implementMethod1(const std::string& number, std::vector<std::string>& factors) {
    if (!method1_pipeline_) {
        return false;
    }
    
    // Allocate buffer for input number
    size_t number_size = number.size();
    auto number_buffer = context_->createBuffer(number_size + 1);
    if (number_buffer->getStatus() != MetalStatus::SUCCESS) {
        return false;
    }
    
    // Copy number to buffer
    number_buffer->copyFromHost(number.c_str(), number_size + 1);
    
    // Allocate buffer for factors
    // Allocate enough space for potential factors
    size_t factors_size = number_size * 10;
    auto factors_buffer = context_->createBuffer(factors_size);
    if (factors_buffer->getStatus() != MetalStatus::SUCCESS) {
        return false;
    }
    
    // Initialize factors buffer to zeros
    std::vector<char> zeros(factors_size, 0);
    factors_buffer->copyFromHost(zeros.data(), factors_size);
    
    // Set buffers
    method1_pipeline_->setBuffer(*number_buffer, 0);
    method1_pipeline_->setBuffer(*factors_buffer, 1);
    
    // Set sizes
    method1_pipeline_->setBytes(number_size, 2);
    method1_pipeline_->setBytes(factors_size, 3);
    
    // Create command queue
    auto command_queue = context_->createCommandQueue();
    if (command_queue->getStatus() != MetalStatus::SUCCESS) {
        return false;
    }
    
    // Dispatch compute
    size_t grid_width = 1024;
    
    auto kernel_start = std::chrono::high_resolution_clock::now();
    method1_pipeline_->dispatch(*command_queue, grid_width);
    auto kernel_end = std::chrono::high_resolution_clock::now();
    
    // Copy factors back
    std::vector<char> factors_buffer_data(factors_size);
    
    auto transfer_start = std::chrono::high_resolution_clock::now();
    factors_buffer->copyToHost(factors_buffer_data.data(), factors_size);
    auto transfer_end = std::chrono::high_resolution_clock::now();
    
    // Parse factors
    factors.clear();
    const char* ptr = factors_buffer_data.data();
    while (*ptr) {
        factors.push_back(ptr);
        ptr += strlen(ptr) + 1;
    }
    
    if (performance_logging_enabled_) {
        PerformanceMetrics metrics;
        metrics.kernel_execution_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(kernel_end - kernel_start).count();
        metrics.memory_transfer_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(transfer_end - transfer_start).count();
        metrics.memory_used_bytes = number_size + 1 + factors_size;
        metrics.grid_width = grid_width;
        metrics.grid_height = 1;
        metrics.grid_depth = 1;
        metrics.method_name = "Method1_ExpandedQFactorization_Detail";
        logPerformance(metrics);
    }
    
    return true;
}

bool MFPMetal::implementMethod2(const std::string& number, std::vector<std::string>& factors) {
    if (!method2_pipeline_) {
        return false;
    }
    
    // Allocate buffer for input number
    size_t number_size = number.size();
    auto number_buffer = context_->createBuffer(number_size + 1);
    if (number_buffer->getStatus() != MetalStatus::SUCCESS) {
        return false;
    }
    
    // Copy number to buffer
    number_buffer->copyFromHost(number.c_str(), number_size + 1);
    
    // Allocate buffer for factors
    // Allocate enough space for potential factors
    size_t factors_size = number_size * 10;
    auto factors_buffer = context_->createBuffer(factors_size);
    if (factors_buffer->getStatus() != MetalStatus::SUCCESS) {
        return false;
    }
    
    // Initialize factors buffer to zeros
    std::vector<char> zeros(factors_size, 0);
    factors_buffer->copyFromHost(zeros.data(), factors_size);
    
    // Set buffers
    method2_pipeline_->setBuffer(*number_buffer, 0);
    method2_pipeline_->setBuffer(*factors_buffer, 1);
    
    // Set sizes
    method2_pipeline_->setBytes(number_size, 2);
    method2_pipeline_->setBytes(factors_size, 3);
    
    // Create command queue
    auto command_queue = context_->createCommandQueue();
    if (command_queue->getStatus() != MetalStatus::SUCCESS) {
        return false;
    }
    
    // Dispatch compute
    size_t grid_width = 1024;
    
    auto kernel_start = std::chrono::high_resolution_clock::now();
    method2_pipeline_->dispatch(*command_queue, grid_width);
    auto kernel_end = std::chrono::high_resolution_clock::now();
    
    // Copy factors back
    std::vector<char> factors_buffer_data(factors_size);
    
    auto transfer_start = std::chrono::high_resolution_clock::now();
    factors_buffer->copyToHost(factors_buffer_data.data(), factors_size);
    auto transfer_end = std::chrono::high_resolution_clock::now();
    
    // Parse factors
    factors.clear();
    const char* ptr = factors_buffer_data.data();
    while (*ptr) {
        factors.push_back(ptr);
        ptr += strlen(ptr) + 1;
    }
    
    if (performance_logging_enabled_) {
        PerformanceMetrics metrics;
        metrics.kernel_execution_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(kernel_end - kernel_start).count();
        metrics.memory_transfer_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(transfer_end - transfer_start).count();
        metrics.memory_used_bytes = number_size + 1 + factors_size;
        metrics.grid_width = grid_width;
        metrics.grid_height = 1;
        metrics.grid_depth = 1;
        metrics.method_name = "Method2_UltrafastWithStructuralFilter_Detail";
        logPerformance(metrics);
    }
    
    return true;
}

bool MFPMetal::implementMethod3(const std::string& number, std::vector<std::string>& factors) {
    if (!method3_pipeline_) {
        return false;
    }
    
    // Allocate buffer for input number
    size_t number_size = number.size();
    auto number_buffer = context_->createBuffer(number_size + 1);
    if (number_buffer->getStatus() != MetalStatus::SUCCESS) {
        return false;
    }
    
    // Copy number to buffer
    number_buffer->copyFromHost(number.c_str(), number_size + 1);
    
    // Allocate buffer for factors
    // Allocate enough space for potential factors
    size_t factors_size = number_size * 10;
    auto factors_buffer = context_->createBuffer(factors_size);
    if (factors_buffer->getStatus() != MetalStatus::SUCCESS) {
        return false;
    }
    
    // Initialize factors buffer to zeros
    std::vector<char> zeros(factors_size, 0);
    factors_buffer->copyFromHost(zeros.data(), factors_size);
    
    // Set buffers
    method3_pipeline_->setBuffer(*number_buffer, 0);
    method3_pipeline_->setBuffer(*factors_buffer, 1);
    
    // Set sizes
    method3_pipeline_->setBytes(number_size, 2);
    method3_pipeline_->setBytes(factors_size, 3);
    
    // Create command queue
    auto command_queue = context_->createCommandQueue();
    if (command_queue->getStatus() != MetalStatus::SUCCESS) {
        return false;
    }
    
    // Dispatch compute
    size_t grid_width = 1024;
    
    auto kernel_start = std::chrono::high_resolution_clock::now();
    method3_pipeline_->dispatch(*command_queue, grid_width);
    auto kernel_end = std::chrono::high_resolution_clock::now();
    
    // Copy factors back
    std::vector<char> factors_buffer_data(factors_size);
    
    auto transfer_start = std::chrono::high_resolution_clock::now();
    factors_buffer->copyToHost(factors_buffer_data.data(), factors_size);
    auto transfer_end = std::chrono::high_resolution_clock::now();
    
    // Parse factors
    factors.clear();
    const char* ptr = factors_buffer_data.data();
    while (*ptr) {
        factors.push_back(ptr);
        ptr += strlen(ptr) + 1;
    }
    
    if (performance_logging_enabled_) {
        PerformanceMetrics metrics;
        metrics.kernel_execution_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(kernel_end - kernel_start).count();
        metrics.memory_transfer_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(transfer_end - transfer_start).count();
        metrics.memory_used_bytes = number_size + 1 + factors_size;
        metrics.grid_width = grid_width;
        metrics.grid_height = 1;
        metrics.grid_depth = 1;
        metrics.method_name = "Method3_ParallelizedWithDynamicBlocks_Detail";
        logPerformance(metrics);
    }
    
    return true;
}

} // namespace metal
} // namespace mfp
