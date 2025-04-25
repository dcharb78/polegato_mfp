#include "gpu/metal_accelerator.h"
#include "hardware/gpu_detector.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <random>
#include <chrono>

// Include Metal headers if on Apple platform
#ifdef __APPLE__
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#endif

namespace mfp {

// Embedded Metal shader code
// This would normally be in a separate .metal file
// For simplicity, we're using placeholder strings
const char* METAL_SHADER_SOURCE = R"(
#include <metal_stdlib>
using namespace metal;

kernel void isPrime(device const uint* number [[buffer(0)]],
                    device bool* result [[buffer(1)]],
                    uint id [[thread_position_in_grid]])
{
    // Placeholder for actual Metal shader code
    // In a real implementation, this would contain the Metal shader code
    // for the isPrime function
}

kernel void factorize(device const uint* number [[buffer(0)]],
                      device uint* factors [[buffer(1)]],
                      device uint* factorCount [[buffer(2)]],
                      uint id [[thread_position_in_grid]])
{
    // Placeholder for actual Metal shader code
    // In a real implementation, this would contain the Metal shader code
    // for the factorize function
}

kernel void nextPrime(device const uint* number [[buffer(0)]],
                      device uint* nextPrime [[buffer(1)]],
                      uint id [[thread_position_in_grid]])
{
    // Placeholder for actual Metal shader code
    // In a real implementation, this would contain the Metal shader code
    // for the nextPrime function
}
)";

// MetalMemory implementation
MetalMemory::MetalMemory() : m_initialized(false), m_device(nullptr), m_total_memory(0), m_allocated_memory(0) {
}

MetalMemory::~MetalMemory() {
    // Cleanup would happen here in a real implementation
}

bool MetalMemory::initialize(MTLDevice device, size_t max_memory_bytes) {
    if (!device) {
        return false;
    }
    
    m_device = device;
    
#ifdef __APPLE__
    // Get device properties
    id<MTLDevice> mtlDevice = (__bridge id<MTLDevice>)device;
    
    // Determine total memory
    // Note: Metal doesn't provide a direct way to query total GPU memory
    // We'll use a conservative estimate based on system memory
    size_t total_memory = 0;
    
    // On Apple Silicon, GPU shares memory with system
    // On discrete GPUs, we can use the recommended max working set size
    if ([mtlDevice isLowPower]) {
        // Integrated GPU (Apple Silicon)
        // Use a portion of system memory
        uint64_t system_memory = [NSProcessInfo processInfo].physicalMemory;
        total_memory = system_memory / 4;  // Use 25% of system memory
    } else {
        // Discrete GPU
        if (@available(macOS 11.0, iOS 14.0, *)) {
            total_memory = [mtlDevice recommendedMaxWorkingSetSize];
        } else {
            // Fallback for older OS versions
            total_memory = 1024 * 1024 * 1024;  // 1 GB
        }
    }
    
    m_total_memory = (max_memory_bytes > 0 && max_memory_bytes < total_memory) ? 
                     max_memory_bytes : total_memory;
    m_initialized = true;
    return true;
#else
    // Simulate success for testing without Metal
    m_total_memory = max_memory_bytes > 0 ? max_memory_bytes : 4ULL * 1024 * 1024 * 1024;
    m_initialized = true;
    return true;
#endif
}

MTLBuffer MetalMemory::allocate(size_t size_bytes) {
    if (!m_initialized || !m_device) {
        return nullptr;
    }
    
    if (m_allocated_memory + size_bytes > m_total_memory) {
        return nullptr;  // Out of memory
    }
    
#ifdef __APPLE__
    id<MTLDevice> mtlDevice = (__bridge id<MTLDevice>)m_device;
    id<MTLBuffer> buffer = [mtlDevice newBufferWithLength:size_bytes options:MTLResourceStorageModeShared];
    
    if (buffer) {
        m_allocated_memory += size_bytes;
        return (__bridge_retained MTLBuffer)buffer;
    }
    return nullptr;
#else
    // Simulate allocation for testing without Metal
    void* ptr = malloc(size_bytes);
    if (ptr) {
        m_allocated_memory += size_bytes;
    }
    return ptr;
#endif
}

void MetalMemory::free(MTLBuffer buffer) {
    if (!m_initialized || !buffer) {
        return;
    }
    
#ifdef __APPLE__
    // Release the Metal buffer
    CFRelease(buffer);
#else
    // Simulate free for testing without Metal
    free(buffer);
#endif
    
    // Note: In a real implementation, we would track the size of each allocation
    // to properly update m_allocated_memory when freeing memory
}

bool MetalMemory::copyHostToDevice(MTLBuffer buffer, const void* host_ptr, size_t size_bytes) {
    if (!m_initialized || !buffer || !host_ptr) {
        return false;
    }
    
#ifdef __APPLE__
    id<MTLBuffer> mtlBuffer = (__bridge id<MTLBuffer>)buffer;
    memcpy([mtlBuffer contents], host_ptr, size_bytes);
    return true;
#else
    // Simulate copy for testing without Metal
    memcpy(buffer, host_ptr, size_bytes);
    return true;
#endif
}

bool MetalMemory::copyDeviceToHost(void* host_ptr, MTLBuffer buffer, size_t size_bytes) {
    if (!m_initialized || !buffer || !host_ptr) {
        return false;
    }
    
#ifdef __APPLE__
    id<MTLBuffer> mtlBuffer = (__bridge id<MTLBuffer>)buffer;
    memcpy(host_ptr, [mtlBuffer contents], size_bytes);
    return true;
#else
    // Simulate copy for testing without Metal
    memcpy(host_ptr, buffer, size_bytes);
    return true;
#endif
}

size_t MetalMemory::getAvailableMemory() const {
    return m_total_memory - m_allocated_memory;
}

size_t MetalMemory::getTotalMemory() const {
    return m_total_memory;
}

// MetalShader implementation
MetalShader::MetalShader() : m_pipeline_state(nullptr) {
}

MetalShader::~MetalShader() {
#ifdef __APPLE__
    if (m_pipeline_state) {
        CFRelease(m_pipeline_state);
        m_pipeline_state = nullptr;
    }
#endif
}

bool MetalShader::loadFromSource(MTLDevice device, const std::string& source, const std::string& function_name) {
#ifdef __APPLE__
    if (m_pipeline_state) {
        CFRelease(m_pipeline_state);
        m_pipeline_state = nullptr;
    }
    
    id<MTLDevice> mtlDevice = (__bridge id<MTLDevice>)device;
    
    // Create library from source
    NSError* error = nil;
    id<MTLLibrary> library = [mtlDevice newLibraryWithSource:[NSString stringWithUTF8String:source.c_str()]
                                                     options:nil
                                                       error:&error];
    if (!library) {
        NSLog(@"Failed to create library: %@", error);
        return false;
    }
    
    // Get function
    id<MTLFunction> function = [library newFunctionWithName:[NSString stringWithUTF8String:function_name.c_str()]];
    if (!function) {
        NSLog(@"Failed to find function %s", function_name.c_str());
        return false;
    }
    
    // Create compute pipeline state
    id<MTLComputePipelineState> pipelineState = [mtlDevice newComputePipelineStateWithFunction:function error:&error];
    if (!pipelineState) {
        NSLog(@"Failed to create pipeline state: %@", error);
        return false;
    }
    
    m_pipeline_state = (__bridge_retained MTLComputePipelineState)pipelineState;
    m_function_name = function_name;
    
    return true;
#else
    // Simulate success for testing without Metal
    m_function_name = function_name;
    return true;
#endif
}

bool MetalShader::loadFromLibrary(MTLDevice device, const std::string& function_name) {
#ifdef __APPLE__
    if (m_pipeline_state) {
        CFRelease(m_pipeline_state);
        m_pipeline_state = nullptr;
    }
    
    id<MTLDevice> mtlDevice = (__bridge id<MTLDevice>)device;
    
    // Get default library
    id<MTLLibrary> library = [mtlDevice newDefaultLibrary];
    if (!library) {
        NSLog(@"Failed to get default library");
        return false;
    }
    
    // Get function
    id<MTLFunction> function = [library newFunctionWithName:[NSString stringWithUTF8String:function_name.c_str()]];
    if (!function) {
        NSLog(@"Failed to find function %s", function_name.c_str());
        return false;
    }
    
    // Create compute pipeline state
    NSError* error = nil;
    id<MTLComputePipelineState> pipelineState = [mtlDevice newComputePipelineStateWithFunction:function error:&error];
    if (!pipelineState) {
        NSLog(@"Failed to create pipeline state: %@", error);
        return false;
    }
    
    m_pipeline_state = (__bridge_retained MTLComputePipelineState)pipelineState;
    m_function_name = function_name;
    
    return true;
#else
    // Simulate success for testing without Metal
    m_function_name = function_name;
    return true;
#endif
}

MTLComputePipelineState MetalShader::getPipelineState() const {
    return m_pipeline_state;
}

const std::string& MetalShader::getFunctionName() const {
    return m_function_name;
}

// MetalCommand implementation
MetalCommand::MetalCommand() : m_command_queue(nullptr), m_command_buffer(nullptr), m_compute_encoder(nullptr), m_encoding(false) {
}

MetalCommand::~MetalCommand() {
#ifdef __APPLE__
    if (m_compute_encoder && m_encoding) {
        [(__bridge id<MTLComputeCommandEncoder>)m_compute_encoder endEncoding];
    }
    
    if (m_command_buffer) {
        CFRelease(m_command_buffer);
    }
#endif
}

bool MetalCommand::initialize(MTLCommandQueue command_queue) {
    if (!command_queue) {
        return false;
    }
    
    m_command_queue = command_queue;
    return true;
}

bool MetalCommand::beginEncoding() {
#ifdef __APPLE__
    if (!m_command_queue) {
        return false;
    }
    
    // Create command buffer
    id<MTLCommandQueue> mtlCommandQueue = (__bridge id<MTLCommandQueue>)m_command_queue;
    id<MTLCommandBuffer> commandBuffer = [mtlCommandQueue commandBuffer];
    if (!commandBuffer) {
        return false;
    }
    
    // Create compute encoder
    id<MTLComputeCommandEncoder> computeEncoder = [commandBuffer computeCommandEncoder];
    if (!computeEncoder) {
        return false;
    }
    
    if (m_command_buffer) {
        CFRelease(m_command_buffer);
    }
    
    m_command_buffer = (__bridge_retained MTLCommandBuffer)commandBuffer;
    m_compute_encoder = (__bridge MTLComputeCommandEncoder)computeEncoder;
    m_encoding = true;
    
    return true;
#else
    // Simulate success for testing without Metal
    m_encoding = true;
    return true;
#endif
}

void MetalCommand::setComputePipelineState(MTLComputePipelineState pipeline_state) {
#ifdef __APPLE__
    if (!m_compute_encoder || !m_encoding || !pipeline_state) {
        return;
    }
    
    id<MTLComputeCommandEncoder> computeEncoder = (__bridge id<MTLComputeCommandEncoder>)m_compute_encoder;
    id<MTLComputePipelineState> mtlPipelineState = (__bridge id<MTLComputePipelineState>)pipeline_state;
    
    [computeEncoder setComputePipelineState:mtlPipelineState];
#endif
}

void MetalCommand::setBuffer(MTLBuffer buffer, int index) {
#ifdef __APPLE__
    if (!m_compute_encoder || !m_encoding || !buffer) {
        return;
    }
    
    id<MTLComputeCommandEncoder> computeEncoder = (__bridge id<MTLComputeCommandEncoder>)m_compute_encoder;
    id<MTLBuffer> mtlBuffer = (__bridge id<MTLBuffer>)buffer;
    
    [computeEncoder setBuffer:mtlBuffer offset:0 atIndex:index];
#endif
}

void MetalCommand::dispatch(int width, int height, int depth) {
#ifdef __APPLE__
    if (!m_compute_encoder || !m_encoding) {
        return;
    }
    
    id<MTLComputeCommandEncoder> computeEncoder = (__bridge id<MTLComputeCommandEncoder>)m_compute_encoder;
    id<MTLComputePipelineState> pipelineState = [computeEncoder computePipelineState];
    
    // Calculate threadgroup size
    NSUInteger threadGroupSizeX = [pipelineState maxTotalThreadsPerThreadgroup];
    if (threadGroupSizeX > 32) {
        threadGroupSizeX = 32;  // Use a reasonable default
    }
    
    MTLSize threadgroupSize = MTLSizeMake(threadGroupSizeX, 1, 1);
    MTLSize gridSize = MTLSizeMake(width, height, depth);
    
    [computeEncoder dispatchThreads:gridSize threadsPerThreadgroup:threadgroupSize];
#endif
}

void MetalCommand::endEncoding() {
#ifdef __APPLE__
    if (!m_compute_encoder || !m_encoding) {
        return;
    }
    
    id<MTLComputeCommandEncoder> computeEncoder = (__bridge id<MTLComputeCommandEncoder>)m_compute_encoder;
    [computeEncoder endEncoding];
    
    m_encoding = false;
#else
    m_encoding = false;
#endif
}

bool MetalCommand::commitAndWait() {
#ifdef __APPLE__
    if (!m_command_buffer) {
        return false;
    }
    
    id<MTLCommandBuffer> commandBuffer = (__bridge id<MTLCommandBuffer>)m_command_buffer;
    
    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];
    
    return [commandBuffer status] == MTLCommandBufferStatusCompleted;
#else
    // Simulate success for testing without Metal
    return true;
#endif
}

// MetalAccelerator implementation
MetalAccelerator::MetalAccelerator() : m_initialized(false), m_device(nullptr), m_command_queue(nullptr) {
    m_shader_is_prime = std::make_unique<MetalShader>();
    m_shader_factorize = std::make_unique<MetalShader>();
    m_shader_next_prime = std::make_unique<MetalShader>();
}

MetalAccelerator::~MetalAccelerator() {
#ifdef __APPLE__
    if (m_command_queue) {
        CFRelease(m_command_queue);
    }
    
    if (m_device) {
        CFRelease(m_device);
    }
#endif
}

bool MetalAccelerator::initialize(const GPUInfo& gpu_info) {
    // Check if Metal is supported
    if (!gpu_info.getAPIs().supports_metal) {
        return false;
    }
    
#ifdef __APPLE__
    // Get default Metal device
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (!device) {
        return false;
    }
    
    // Create command queue
    id<MTLCommandQueue> commandQueue = [device newCommandQueue];
    if (!commandQueue) {
        return false;
    }
    
    m_device = (__bridge_retained MTLDevice)device;
    m_command_queue = (__bridge_retained MTLCommandQueue)commandQueue;
    
    // Initialize memory system
    if (!m_memory.initialize(m_device)) {
        return false;
    }
    
    // Load shaders
    if (!loadShaders()) {
        return false;
    }
    
    m_initialized = true;
    return true;
#else
    // Simulate success for testing without Metal
    m_initialized = true;
    return true;
#endif
}

bool MetalAccelerator::isAvailable() const {
    return m_initialized;
}

std::string MetalAccelerator::getDeviceProperties() const {
    if (!m_initialized) {
        return "Metal not initialized";
    }
    
    std::stringstream ss;
    ss << "Metal Device Properties:" << std::endl;
    
#ifdef __APPLE__
    id<MTLDevice> device = (__bridge id<MTLDevice>)m_device;
    
    ss << "  Name: " << [[device name] UTF8String] << std::endl;
    ss << "  Registry ID: 0x" << std::hex << [device registryID] << std::dec << std::endl;
    ss << "  Low Power: " << ([device isLowPower] ? "Yes" : "No") << std::endl;
    ss << "  Headless: " << ([device isHeadless] ? "Yes" : "No") << std::endl;
    ss << "  Removable: " << ([device isRemovable] ? "Yes" : "No") << std::endl;
    
    if (@available(macOS 10.15, iOS 13.0, *)) {
        ss << "  Has Unified Memory: " << ([device hasUnifiedMemory] ? "Yes" : "No") << std::endl;
    }
    
    if (@available(macOS 11.0, iOS 14.0, *)) {
        ss << "  Recommended Max Working Set Size: " << ([device recommendedMaxWorkingSetSize] / (1024 * 1024)) << " MB" << std::endl;
    }
#endif
    
    ss << "  Total Memory: " << (m_memory.getTotalMemory() / (1024 * 1024)) << " MB" << std::endl;
    ss << "  Available Memory: " << (m_memory.getAvailableMemory() / (1024 * 1024)) << " MB" << std::endl;
    
    return ss.str();
}

bool MetalAccelerator::isPrime(const mpz_t number, bool& result) {
    if (!m_initialized) {
        return false;
    }
    
    // Convert mpz_t to a format suitable for Metal
    size_t buffer_size = mpz_sizeinbase(number, 2) / 8 + 1;
    MTLBuffer d_number = m_memory.allocate(buffer_size);
    MTLBuffer d_result = m_memory.allocate(sizeof(bool));
    
    if (!d_number || !d_result) {
        if (d_number) m_memory.free(d_number);
        if (d_result) m_memory.free(d_result);
        return false;
    }
    
    // Copy number to device
    unsigned char* h_number = new unsigned char[buffer_size];
    mpz_export(h_number, nullptr, 1, 1, 0, 0, number);
    m_memory.copyHostToDevice(d_number, h_number, buffer_size);
    delete[] h_number;
    
    // Initialize result to false
    bool h_result = false;
    m_memory.copyHostToDevice(d_result, &h_result, sizeof(bool));
    
    // Create command
    MetalCommand command;
    if (!command.initialize(m_command_queue)) {
        m_memory.free(d_number);
        m_memory.free(d_result);
        return false;
    }
    
    // Configure and dispatch
    int width, height, depth;
    configureThreadGroups(buffer_size, width, height, depth);
    
    if (!command.beginEncoding()) {
        m_memory.free(d_number);
        m_memory.free(d_result);
        return false;
    }
    
    command.setComputePipelineState(m_shader_is_prime->getPipelineState());
    command.setBuffer(d_number, 0);
    command.setBuffer(d_result, 1);
    command.dispatch(width, height, depth);
    command.endEncoding();
    
    if (!command.commitAndWait()) {
        m_memory.free(d_number);
        m_memory.free(d_result);
        return false;
    }
    
    // Get result
    m_memory.copyDeviceToHost(&result, d_result, sizeof(bool));
    
    // Clean up
    m_memory.free(d_number);
    m_memory.free(d_result);
    
    return true;
}

bool MetalAccelerator::factorize(const mpz_t number, std::vector<mpz_t>& factors) {
    if (!m_initialized) {
        return false;
    }
    
    // Convert mpz_t to a format suitable for Metal
    size_t buffer_size = mpz_sizeinbase(number, 2) / 8 + 1;
    MTLBuffer d_number = m_memory.allocate(buffer_size);
    
    // Allocate space for factors (assume max 1000 factors)
    const int max_factors = 1000;
    size_t factors_buffer_size = max_factors * buffer_size;
    MTLBuffer d_factors = m_memory.allocate(factors_buffer_size);
    MTLBuffer d_factor_count = m_memory.allocate(sizeof(int));
    
    if (!d_number || !d_factors || !d_factor_count) {
        if (d_number) m_memory.free(d_number);
        if (d_factors) m_memory.free(d_factors);
        if (d_factor_count) m_memory.free(d_factor_count);
        return false;
    }
    
    // Copy number to device
    unsigned char* h_number = new unsigned char[buffer_size];
    mpz_export(h_number, nullptr, 1, 1, 0, 0, number);
    m_memory.copyHostToDevice(d_number, h_number, buffer_size);
    delete[] h_number;
    
    // Initialize factor count to 0
    int h_factor_count = 0;
    m_memory.copyHostToDevice(d_factor_count, &h_factor_count, sizeof(int));
    
    // Create command
    MetalCommand command;
    if (!command.initialize(m_command_queue)) {
        m_memory.free(d_number);
        m_memory.free(d_factors);
        m_memory.free(d_factor_count);
        return false;
    }
    
    // Configure and dispatch
    int width, height, depth;
    configureThreadGroups(buffer_size, width, height, depth);
    
    if (!command.beginEncoding()) {
        m_memory.free(d_number);
        m_memory.free(d_factors);
        m_memory.free(d_factor_count);
        return false;
    }
    
    command.setComputePipelineState(m_shader_factorize->getPipelineState());
    command.setBuffer(d_number, 0);
    command.setBuffer(d_factors, 1);
    command.setBuffer(d_factor_count, 2);
    command.dispatch(width, height, depth);
    command.endEncoding();
    
    if (!command.commitAndWait()) {
        m_memory.free(d_number);
        m_memory.free(d_factors);
        m_memory.free(d_factor_count);
        return false;
    }
    
    // Get factor count
    m_memory.copyDeviceToHost(&h_factor_count, d_factor_count, sizeof(int));
    h_factor_count = std::min(h_factor_count, max_factors);
    
    // Copy factors back to host
    unsigned char* h_factors = new unsigned char[factors_buffer_size];
    m_memory.copyDeviceToHost(h_factors, d_factors, factors_buffer_size);
    
    // Convert factors to mpz_t
    factors.clear();
    for (int i = 0; i < h_factor_count; i++) {
        mpz_t factor;
        mpz_init(factor);
        
        unsigned char* factor_data = h_factors + i * buffer_size;
        mpz_import(factor, buffer_size, 1, 1, 0, 0, factor_data);
        
        factors.push_back(factor);
    }
    
    delete[] h_factors;
    
    // Clean up
    m_memory.free(d_number);
    m_memory.free(d_factors);
    m_memory.free(d_factor_count);
    
    return true;
}

bool MetalAccelerator::nextPrime(const mpz_t number, mpz_t next_prime) {
    if (!m_initialized) {
        return false;
    }
    
    // Convert mpz_t to a format suitable for Metal
    size_t buffer_size = mpz_sizeinbase(number, 2) / 8 + 1;
    MTLBuffer d_number = m_memory.allocate(buffer_size);
    MTLBuffer d_next_prime = m_memory.allocate(buffer_size);
    
    if (!d_number || !d_next_prime) {
        if (d_number) m_memory.free(d_number);
        if (d_next_prime) m_memory.free(d_next_prime);
        return false;
    }
    
    // Copy number to device
    unsigned char* h_number = new unsigned char[buffer_size];
    mpz_export(h_number, nullptr, 1, 1, 0, 0, number);
    m_memory.copyHostToDevice(d_number, h_number, buffer_size);
    delete[] h_number;
    
    // Create command
    MetalCommand command;
    if (!command.initialize(m_command_queue)) {
        m_memory.free(d_number);
        m_memory.free(d_next_prime);
        return false;
    }
    
    // Configure and dispatch
    int width, height, depth;
    configureThreadGroups(buffer_size, width, height, depth);
    
    if (!command.beginEncoding()) {
        m_memory.free(d_number);
        m_memory.free(d_next_prime);
        return false;
    }
    
    command.setComputePipelineState(m_shader_next_prime->getPipelineState());
    command.setBuffer(d_number, 0);
    command.setBuffer(d_next_prime, 1);
    command.dispatch(width, height, depth);
    command.endEncoding();
    
    if (!command.commitAndWait()) {
        m_memory.free(d_number);
        m_memory.free(d_next_prime);
        return false;
    }
    
    // Copy next prime back to host
    unsigned char* h_next_prime = new unsigned char[buffer_size];
    m_memory.copyDeviceToHost(h_next_prime, d_next_prime, buffer_size);
    
    // Convert to mpz_t
    mpz_import(next_prime, buffer_size, 1, 1, 0, 0, h_next_prime);
    delete[] h_next_prime;
    
    // Clean up
    m_memory.free(d_number);
    m_memory.free(d_next_prime);
    
    return true;
}

double MetalAccelerator::benchmark(const mpz_t number) {
    if (!m_initialized) {
        return -1.0;
    }
    
    // Measure time for isPrime operation
    auto start = std::chrono::high_resolution_clock::now();
    
    bool is_prime_result;
    bool success = isPrime(number, is_prime_result);
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;
    
    if (!success) {
        return -1.0;
    }
    
    return elapsed.count();
}

bool MetalAccelerator::loadShaders() {
#ifdef __APPLE__
    // Load isPrime shader
    if (!m_shader_is_prime->loadFromSource(m_device, METAL_SHADER_SOURCE, "isPrime")) {
        return false;
    }
    
    // Load factorize shader
    if (!m_shader_factorize->loadFromSource(m_device, METAL_SHADER_SOURCE, "factorize")) {
        return false;
    }
    
    // Load nextPrime shader
    if (!m_shader_next_prime->loadFromSource(m_device, METAL_SHADER_SOURCE, "nextPrime")) {
        return false;
    }
    
    return true;
#else
    // Simulate success for testing without Metal
    return true;
#endif
}

bool MetalAccelerator::configureThreadGroups(size_t data_size, int& width, int& height, int& depth) {
    // Configure thread groups based on data size
    // This is a simplified approach; in a real implementation, we would
    // consider the device's capabilities and other factors
    
    width = data_size;
    height = 1;
    depth = 1;
    
    return true;
}

// Factory method implementation
std::unique_ptr<MFPBase> createMetalMFP(int method_number) {
    // This would create an MFP implementation that uses Metal for acceleration
    // For now, we'll return nullptr to indicate that Metal is not available
    return nullptr;
}

} // namespace mfp
