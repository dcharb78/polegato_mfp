#include "gpu_detector.h"
#include "cpu_detector.h"
#include "memory_storage_detector.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <thread>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <regex>

// Platform-specific includes
#ifdef _WIN32
#include <windows.h>
#include <setupapi.h>
#include <devguid.h>
#include <regstr.h>
#elif defined(__APPLE__)
#include <sys/sysctl.h>
#include <IOKit/IOKitLib.h>
#include <CoreFoundation/CoreFoundation.h>
#else
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

// CUDA includes (if available)
#if defined(HAVE_CUDA)
#include <cuda_runtime.h>
#include <nvml.h>
#endif

// OpenCL includes (if available)
#if defined(HAVE_OPENCL)
#include <CL/cl.h>
#endif

// Metal includes (if available on macOS)
#if defined(__APPLE__) && defined(HAVE_METAL)
#include <Metal/Metal.h>
#endif

namespace mfp {
namespace system {

GPUDetector::GPUDetector() {
    // Initialize with default values
}

GPUDetector::~GPUDetector() {
    // Clean up resources if needed
}

bool GPUDetector::detect() {
    if (initialized_) {
        return true; // Already initialized
    }
    
    bool success = true;
    gpu_info_.clear();
    
    // Detect GPUs based on platform
#if defined(_WIN32)
    success = detectGPUsWindows();
#elif defined(__APPLE__)
    success = detectGPUsMacOS();
#else
    success = detectGPUsLinux();
#endif
    
    // Try to detect GPUs using specific APIs
    // These methods will add to the existing gpu_info_ vector
    detectCUDAGPUs();
    detectOpenCLGPUs();
    detectMetalGPUs();
    detectVulkanGPUs();
    detectDirectXGPUs();
    
    // Deduplicate GPUs
    std::vector<GPUInfo> unique_gpus;
    for (const auto& gpu : gpu_info_) {
        bool is_duplicate = false;
        for (const auto& unique_gpu : unique_gpus) {
            // Check if this GPU is already in the list
            // Compare by name and device ID
            if (gpu.name == unique_gpu.name && gpu.device_id == unique_gpu.device_id) {
                is_duplicate = true;
                break;
            }
        }
        
        if (!is_duplicate) {
            unique_gpus.push_back(gpu);
        }
    }
    
    gpu_info_ = std::move(unique_gpus);
    
    initialized_ = success && !gpu_info_.empty();
    return initialized_;
}

const std::vector<GPUInfo>& GPUDetector::getGPUInfo() const {
    return gpu_info_;
}

int GPUDetector::getGPUCount() const {
    return static_cast<int>(gpu_info_.size());
}

const GPUInfo* GPUDetector::getGPUByIndex(int index) const {
    if (index >= 0 && index < static_cast<int>(gpu_info_.size())) {
        return &gpu_info_[index];
    }
    return nullptr;
}

const GPUInfo* GPUDetector::getBestGPUForCompute() const {
    if (gpu_info_.empty()) {
        return nullptr;
    }
    
    // Find the best GPU for compute
    const GPUInfo* best_gpu = nullptr;
    double best_score = 0.0;
    
    for (const auto& gpu : gpu_info_) {
        // Calculate a score based on compute capabilities
        double compute_score = 0.0;
        
        // CUDA cores/compute units
        compute_score += gpu.compute_info.cuda_cores * 0.01;
        compute_score += gpu.compute_info.compute_units * 10.0;
        
        // Tensor cores (for NVIDIA)
        compute_score += gpu.compute_info.tensor_cores * 0.5;
        
        // Clock speed
        compute_score += gpu.compute_info.core_clock_mhz * 0.01;
        
        // Memory
        compute_score += (gpu.memory_info.total_memory_bytes / (1024.0 * 1024.0 * 1024.0)) * 100.0; // GB
        compute_score += gpu.memory_info.memory_bandwidth_gbps * 10.0;
        
        // Theoretical performance
        compute_score += gpu.compute_info.theoretical_tflops_fp32 * 1000.0;
        
        // API support
        bool has_cuda = false;
        bool has_metal = false;
        for (const auto& api : gpu.api_support) {
            if (api == GPUAPISupport::CUDA) {
                has_cuda = true;
                compute_score += 500.0; // Prefer CUDA
            } else if (api == GPUAPISupport::METAL) {
                has_metal = true;
                compute_score += 400.0; // Metal is also good
            } else if (api == GPUAPISupport::OPENCL) {
                compute_score += 300.0;
            }
        }
        
        // Integrated GPUs are less preferred
        if (gpu.is_integrated) {
            compute_score *= 0.5;
        }
        
        // Update best GPU if this one has a higher score
        if (best_gpu == nullptr || compute_score > best_score) {
            best_gpu = &gpu;
            best_score = compute_score;
        }
    }
    
    return best_gpu;
}

bool GPUDetector::hasCUDASupport() const {
    for (const auto& gpu : gpu_info_) {
        for (const auto& api : gpu.api_support) {
            if (api == GPUAPISupport::CUDA) {
                return true;
            }
        }
    }
    return false;
}

bool GPUDetector::hasMetalSupport() const {
    for (const auto& gpu : gpu_info_) {
        for (const auto& api : gpu.api_support) {
            if (api == GPUAPISupport::METAL) {
                return true;
            }
        }
    }
    return false;
}

std::string GPUDetector::getSummary() const {
    if (!initialized_) {
        return "GPU detection not initialized";
    }
    
    std::stringstream ss;
    ss << "GPU Information:\n";
    ss << "  Number of GPUs: " << gpu_info_.size() << "\n\n";
    
    for (size_t i = 0; i < gpu_info_.size(); ++i) {
        const auto& gpu = gpu_info_[i];
        ss << "  GPU " << (i + 1) << ":\n";
        ss << "    Name: " << gpu.name << "\n";
        ss << "    Vendor: " << static_cast<int>(gpu.vendor) << " (";
        
        switch (gpu.vendor) {
            case GPUVendor::NVIDIA: ss << "NVIDIA"; break;
            case GPUVendor::AMD: ss << "AMD"; break;
            case GPUVendor::INTEL: ss << "Intel"; break;
            case GPUVendor::APPLE: ss << "Apple"; break;
            default: ss << "Unknown"; break;
        }
        
        ss << ")\n";
        
        ss << "    Architecture: " << static_cast<int>(gpu.architecture) << "\n";
        ss << "    Driver Version: " << gpu.driver_version << "\n";
        ss << "    API Support:";
        for (const auto& api : gpu.api_support) {
            switch (api) {
                case GPUAPISupport::CUDA: ss << " CUDA"; break;
                case GPUAPISupport::OPENCL: ss << " OpenCL"; break;
                case GPUAPISupport::METAL: ss << " Metal"; break;
                case GPUAPISupport::DIRECTX: ss << " DirectX"; break;
                case GPUAPISupport::VULKAN: ss << " Vulkan"; break;
                default: break;
            }
        }
        ss << "\n";
        
        ss << "    Memory: " << (gpu.memory_info.total_memory_bytes / (1024 * 1024 * 1024)) << " GB";
        if (gpu.memory_info.memory_bandwidth_gbps > 0) {
            ss << ", " << gpu.memory_info.memory_bandwidth_gbps << " GB/s";
        }
        ss << "\n";
        
        if (gpu.compute_info.cuda_cores > 0) {
            ss << "    CUDA Cores: " << gpu.compute_info.cuda_cores << "\n";
        }
        
        if (gpu.compute_info.tensor_cores > 0) {
            ss << "    Tensor Cores: " << gpu.compute_info.tensor_cores << "\n";
        }
        
        if (gpu.compute_info.compute_units > 0) {
            ss << "    Compute Units: " << gpu.compute_info.compute_units << "\n";
        }
        
        if (gpu.compute_info.core_clock_mhz > 0) {
            ss << "    Core Clock: " << gpu.compute_info.core_clock_mhz << " MHz\n";
        }
        
        if (gpu.compute_info.theoretical_tflops_fp32 > 0) {
            ss << "    FP32 Performance: " << gpu.compute_info.theoretical_tflops_fp32 << " TFLOPS\n";
        }
        
        if (!gpu.compute_info.cuda_compute_capability.empty()) {
            ss << "    CUDA Compute Capability: " << gpu.compute_info.cuda_compute_capability << "\n";
        }
        
        ss << "    Integrated: " << (gpu.is_integrated ? "Yes" : "No") << "\n";
        
        if (gpu.pcie_generation > 0 && gpu.pcie_lanes > 0) {
            ss << "    PCIe: Gen " << gpu.pcie_generation << " x" << gpu.pcie_lanes << "\n";
        }
        
        if (gpu.power_usage_watts > 0) {
            ss << "    Power Usage: " << gpu.power_usage_watts << " W";
            if (gpu.max_power_watts > 0) {
                ss << " / " << gpu.max_power_watts << " W";
            }
            ss << "\n";
        }
        
        if (gpu.temperature_celsius > 0) {
            ss << "    Temperature: " << gpu.temperature_celsius << " °C\n";
        }
        
        ss << "\n";
    }
    
    return ss.str();
}

void GPUDetector::setCPUDetector(const CPUDetector* cpu_detector) {
    cpu_detector_ = cpu_detector;
}

void GPUDetector::setMemoryDetector(const MemoryDetector* memory_detector) {
    memory_detector_ = memory_detector;
}

bool GPUDetector::detectGPUsWindows() {
#ifdef _WIN32
    // Try to detect GPUs using WMI
    std::string output;
    if (executeCommand("wmic path win32_VideoController get Name, AdapterRAM, DriverVersion /format:list", output)) {
        std::istringstream iss(output);
        std::string line;
        
        GPUInfo current_gpu;
        bool has_gpu = false;
        
        while (std::getline(iss, line)) {
            // Remove carriage return if present
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            
            if (line.empty()) {
                // Empty line indicates end of current GPU
                if (has_gpu) {
                    // Add current GPU to list
                    current_gpu.device_id = static_cast<int>(gpu_info_.size());
                    gpu_info_.push_back(current_gpu);
                    
                    // Reset for next GPU
                    current_gpu = GPUInfo();
                    has_gpu = false;
                }
                continue;
            }
            
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                
                if (key == "Name") {
                    current_gpu.name = value;
                    has_gpu = true;
                    
                    // Try to determine vendor and architecture from name
                    if (value.find("NVIDIA") != std::string::npos) {
                        current_gpu.vendor = GPUVendor::NVIDIA;
                        current_gpu.architecture = determineNVIDIAArchitecture(value, "");
                        current_gpu.api_support.push_back(GPUAPISupport::CUDA);
                    } else if (value.find("AMD") != std::string::npos || value.find("Radeon") != std::string::npos) {
                        current_gpu.vendor = GPUVendor::AMD;
                        current_gpu.architecture = determineAMDArchitecture(value);
                    } else if (value.find("Intel") != std::string::npos) {
                        current_gpu.vendor = GPUVendor::INTEL;
                        current_gpu.architecture = determineIntelArchitecture(value);
                        current_gpu.is_integrated = true;
                    }
                    
                    // Add common API support
                    current_gpu.api_support.push_back(GPUAPISupport::DIRECTX);
                    current_gpu.api_support.push_back(GPUAPISupport::VULKAN);
                    current_gpu.api_support.push_back(GPUAPISupport::OPENCL);
                } else if (key == "AdapterRAM") {
                    try {
                        current_gpu.memory_info.total_memory_bytes = std::stoull(value);
                    } catch (...) {
                        // Ignore parsing errors
                    }
                } else if (key == "DriverVersion") {
                    current_gpu.driver_version = value;
                }
            }
        }
        
        // Add last GPU if any
        if (has_gpu) {
            current_gpu.device_id = static_cast<int>(gpu_info_.size());
            gpu_info_.push_back(current_gpu);
        }
        
        return !gpu_info_.empty();
    }
#endif
    
    return false;
}

bool GPUDetector::detectGPUsMacOS() {
#ifdef __APPLE__
    // On macOS, we can use IOKit to get GPU information
    CFMutableDictionaryRef matching = IOServiceMatching("IOPCIDevice");
    if (!matching) {
        return false;
    }
    
    io_iterator_t iterator;
    if (IOServiceGetMatchingServices(kIOMasterPortDefault, matching, &iterator) != kIOReturnSuccess) {
        return false;
    }
    
    io_object_t device;
    while ((device = IOIteratorNext(iterator)) != 0) {
        CFMutableDictionaryRef properties = NULL;
        if (IORegistryEntryCreateCFProperties(device, &properties, kCFAllocatorDefault, kNilOptions) != kIOReturnSuccess) {
            IOObjectRelease(device);
            continue;
        }
        
        // Check if this is a GPU
        CFDataRef model = (CFDataRef)CFDictionaryGetValue(properties, CFSTR("model"));
        CFDataRef name = (CFDataRef)CFDictionaryGetValue(properties, CFSTR("name"));
        CFNumberRef vendor_id = (CFNumberRef)CFDictionaryGetValue(properties, CFSTR("vendor-id"));
        CFNumberRef device_id = (CFNumberRef)CFDictionaryGetValue(properties, CFSTR("device-id"));
        
        if (model || name) {
            // This might be a GPU
            GPUInfo gpu;
            gpu.device_id = static_cast<int>(gpu_info_.size());
            
            // Get name
            if (name) {
                const char* name_str = (const char*)CFDataGetBytePtr(name);
                size_t name_len = CFDataGetLength(name);
                gpu.name = std::string(name_str, name_len);
            } else if (model) {
                const char* model_str = (const char*)CFDataGetBytePtr(model);
                size_t model_len = CFDataGetLength(model);
                gpu.name = std::string(model_str, model_len);
            }
            
            // Get vendor ID
            if (vendor_id) {
                uint32_t vendor_id_value;
                if (CFNumberGetValue(vendor_id, kCFNumberSInt32Type, &vendor_id_value)) {
                    if (vendor_id_value == 0x10DE) {
                        gpu.vendor = GPUVendor::NVIDIA;
                    } else if (vendor_id_value == 0x1002) {
                        gpu.vendor = GPUVendor::AMD;
                    } else if (vendor_id_value == 0x8086) {
                        gpu.vendor = GPUVendor::INTEL;
                    } else if (vendor_id_value == 0x106B) {
                        gpu.vendor = GPUVendor::APPLE;
                    }
                }
            }
            
            // Check if this is an Apple Silicon GPU
            if (gpu.name.find("Apple") != std::string::npos) {
                gpu.vendor = GPUVendor::APPLE;
                gpu.architecture = determineAppleArchitecture(gpu.name);
                gpu.api_support.push_back(GPUAPISupport::METAL);
                gpu.is_integrated = true;
            } else if (gpu.vendor == GPUVendor::NVIDIA) {
                gpu.architecture = determineNVIDIAArchitecture(gpu.name, "");
                gpu.api_support.push_back(GPUAPISupport::OPENCL);
                gpu.api_support.push_back(GPUAPISupport::METAL);
            } else if (gpu.vendor == GPUVendor::AMD) {
                gpu.architecture = determineAMDArchitecture(gpu.name);
                gpu.api_support.push_back(GPUAPISupport::OPENCL);
                gpu.api_support.push_back(GPUAPISupport::METAL);
            } else if (gpu.vendor == GPUVendor::INTEL) {
                gpu.architecture = determineIntelArchitecture(gpu.name);
                gpu.api_support.push_back(GPUAPISupport::OPENCL);
                gpu.api_support.push_back(GPUAPISupport::METAL);
                gpu.is_integrated = true;
            }
            
            // Get memory information
            CFNumberRef vram = (CFNumberRef)CFDictionaryGetValue(properties, CFSTR("VRAM,totalMB"));
            if (vram) {
                uint32_t vram_mb;
                if (CFNumberGetValue(vram, kCFNumberSInt32Type, &vram_mb)) {
                    gpu.memory_info.total_memory_bytes = static_cast<size_t>(vram_mb) * 1024 * 1024;
                }
            }
            
            // Add to list if it's a GPU
            if (gpu.vendor != GPUVendor::UNKNOWN) {
                gpu_info_.push_back(gpu);
            }
        }
        
        CFRelease(properties);
        IOObjectRelease(device);
    }
    
    IOObjectRelease(iterator);
    
    // If we didn't find any GPUs, try using system_profiler
    if (gpu_info_.empty()) {
        std::string output;
        if (executeCommand("system_profiler SPDisplaysDataType", output)) {
            std::istringstream iss(output);
            std::string line;
            
            GPUInfo current_gpu;
            bool has_gpu = false;
            
            while (std::getline(iss, line)) {
                // Check for GPU name
                if (line.find("Chipset Model:") != std::string::npos) {
                    if (has_gpu) {
                        // Add previous GPU to list
                        current_gpu.device_id = static_cast<int>(gpu_info_.size());
                        gpu_info_.push_back(current_gpu);
                        current_gpu = GPUInfo();
                    }
                    
                    size_t pos = line.find(':');
                    if (pos != std::string::npos) {
                        current_gpu.name = line.substr(pos + 1);
                        // Trim whitespace
                        current_gpu.name.erase(0, current_gpu.name.find_first_not_of(" \t"));
                        current_gpu.name.erase(current_gpu.name.find_last_not_of(" \t") + 1);
                        has_gpu = true;
                        
                        // Determine vendor and architecture
                        if (current_gpu.name.find("Apple") != std::string::npos) {
                            current_gpu.vendor = GPUVendor::APPLE;
                            current_gpu.architecture = determineAppleArchitecture(current_gpu.name);
                            current_gpu.api_support.push_back(GPUAPISupport::METAL);
                            current_gpu.is_integrated = true;
                        } else if (current_gpu.name.find("NVIDIA") != std::string::npos) {
                            current_gpu.vendor = GPUVendor::NVIDIA;
                            current_gpu.architecture = determineNVIDIAArchitecture(current_gpu.name, "");
                            current_gpu.api_support.push_back(GPUAPISupport::OPENCL);
                            current_gpu.api_support.push_back(GPUAPISupport::METAL);
                        } else if (current_gpu.name.find("AMD") != std::string::npos || 
                                  current_gpu.name.find("Radeon") != std::string::npos) {
                            current_gpu.vendor = GPUVendor::AMD;
                            current_gpu.architecture = determineAMDArchitecture(current_gpu.name);
                            current_gpu.api_support.push_back(GPUAPISupport::OPENCL);
                            current_gpu.api_support.push_back(GPUAPISupport::METAL);
                        } else if (current_gpu.name.find("Intel") != std::string::npos) {
                            current_gpu.vendor = GPUVendor::INTEL;
                            current_gpu.architecture = determineIntelArchitecture(current_gpu.name);
                            current_gpu.api_support.push_back(GPUAPISupport::OPENCL);
                            current_gpu.api_support.push_back(GPUAPISupport::METAL);
                            current_gpu.is_integrated = true;
                        }
                    }
                } else if (has_gpu) {
                    // Check for memory information
                    if (line.find("VRAM") != std::string::npos) {
                        size_t pos = line.find(':');
                        if (pos != std::string::npos) {
                            std::string vram_str = line.substr(pos + 1);
                            // Trim whitespace
                            vram_str.erase(0, vram_str.find_first_not_of(" \t"));
                            vram_str.erase(vram_str.find_last_not_of(" \t") + 1);
                            
                            // Extract numeric part
                            std::string vram_value;
                            for (char c : vram_str) {
                                if (std::isdigit(c) || c == '.') {
                                    vram_value += c;
                                } else if (!vram_value.empty()) {
                                    break;
                                }
                            }
                            
                            try {
                                double vram_gb = std::stod(vram_value);
                                current_gpu.memory_info.total_memory_bytes = static_cast<size_t>(vram_gb * 1024 * 1024 * 1024);
                            } catch (...) {
                                // Ignore parsing errors
                            }
                        }
                    }
                }
            }
            
            // Add last GPU if any
            if (has_gpu) {
                current_gpu.device_id = static_cast<int>(gpu_info_.size());
                gpu_info_.push_back(current_gpu);
            }
        }
    }
    
    return !gpu_info_.empty();
#else
    return false;
#endif
}

bool GPUDetector::detectGPUsLinux() {
#if !defined(_WIN32) && !defined(__APPLE__)
    // On Linux, we can use lspci to get GPU information
    std::string output;
    if (executeCommand("lspci -v | grep -E 'VGA|3D|Display'", output)) {
        std::istringstream iss(output);
        std::string line;
        
        while (std::getline(iss, line)) {
            GPUInfo gpu;
            gpu.device_id = static_cast<int>(gpu_info_.size());
            
            // Extract GPU name
            if (line.find("NVIDIA") != std::string::npos) {
                gpu.vendor = GPUVendor::NVIDIA;
                gpu.api_support.push_back(GPUAPISupport::CUDA);
                gpu.api_support.push_back(GPUAPISupport::OPENCL);
                gpu.api_support.push_back(GPUAPISupport::VULKAN);
                
                // Extract model name
                size_t pos = line.find("NVIDIA");
                if (pos != std::string::npos) {
                    gpu.name = line.substr(pos);
                    // Trim trailing characters
                    gpu.name.erase(gpu.name.find_last_not_of(" \t\n\r)") + 1);
                }
                
                // Determine architecture
                gpu.architecture = determineNVIDIAArchitecture(gpu.name, "");
            } else if (line.find("AMD") != std::string::npos || line.find("Radeon") != std::string::npos) {
                gpu.vendor = GPUVendor::AMD;
                gpu.api_support.push_back(GPUAPISupport::OPENCL);
                gpu.api_support.push_back(GPUAPISupport::VULKAN);
                
                // Extract model name
                size_t pos = line.find("AMD");
                if (pos != std::string::npos) {
                    gpu.name = line.substr(pos);
                } else {
                    pos = line.find("Radeon");
                    if (pos != std::string::npos) {
                        gpu.name = line.substr(pos);
                    }
                }
                
                // Trim trailing characters
                if (!gpu.name.empty()) {
                    gpu.name.erase(gpu.name.find_last_not_of(" \t\n\r)") + 1);
                }
                
                // Determine architecture
                gpu.architecture = determineAMDArchitecture(gpu.name);
            } else if (line.find("Intel") != std::string::npos) {
                gpu.vendor = GPUVendor::INTEL;
                gpu.api_support.push_back(GPUAPISupport::OPENCL);
                gpu.api_support.push_back(GPUAPISupport::VULKAN);
                gpu.is_integrated = true;
                
                // Extract model name
                size_t pos = line.find("Intel");
                if (pos != std::string::npos) {
                    gpu.name = line.substr(pos);
                    // Trim trailing characters
                    gpu.name.erase(gpu.name.find_last_not_of(" \t\n\r)") + 1);
                }
                
                // Determine architecture
                gpu.architecture = determineIntelArchitecture(gpu.name);
            } else {
                // Unknown vendor, skip
                continue;
            }
            
            // Get more detailed information using lspci -v -s
            std::string pci_address = line.substr(0, line.find(' '));
            std::string detailed_output;
            if (executeCommand("lspci -v -s " + pci_address, detailed_output)) {
                std::istringstream detailed_iss(detailed_output);
                std::string detailed_line;
                
                while (std::getline(detailed_iss, detailed_line)) {
                    // Check for memory information
                    if (detailed_line.find("Memory") != std::string::npos && 
                        detailed_line.find("prefetchable") != std::string::npos) {
                        // Extract memory size
                        std::regex memory_regex("\\[size=(\\d+)([KMG])\\]");
                        std::smatch match;
                        if (std::regex_search(detailed_line, match, memory_regex) && match.size() > 2) {
                            try {
                                size_t memory_size = std::stoull(match[1].str());
                                char unit = match[2].str()[0];
                                
                                if (unit == 'K') {
                                    memory_size *= 1024;
                                } else if (unit == 'M') {
                                    memory_size *= 1024 * 1024;
                                } else if (unit == 'G') {
                                    memory_size *= 1024 * 1024 * 1024;
                                }
                                
                                gpu.memory_info.total_memory_bytes = memory_size;
                            } catch (...) {
                                // Ignore parsing errors
                            }
                        }
                    }
                }
            }
            
            // Add to list
            gpu_info_.push_back(gpu);
        }
    }
    
    // Try to get more information using nvidia-smi for NVIDIA GPUs
    if (executeCommand("nvidia-smi --query-gpu=name,memory.total,driver_version,pci.bus_id,temperature.gpu,power.draw,power.limit,clocks.current.graphics,clocks.max.graphics --format=csv,noheader,nounits", output)) {
        std::istringstream iss(output);
        std::string line;
        
        while (std::getline(iss, line)) {
            std::vector<std::string> values;
            std::istringstream line_iss(line);
            std::string value;
            
            // Parse CSV line
            while (std::getline(line_iss, value, ',')) {
                // Trim whitespace
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);
                values.push_back(value);
            }
            
            if (values.size() >= 9) {
                // Find matching GPU in our list
                for (auto& gpu : gpu_info_) {
                    if (gpu.vendor == GPUVendor::NVIDIA && gpu.name.find(values[0]) != std::string::npos) {
                        // Update with more detailed information
                        gpu.name = values[0];
                        
                        try {
                            // Memory in MiB
                            gpu.memory_info.total_memory_bytes = std::stoull(values[1]) * 1024 * 1024;
                            
                            // Driver version
                            gpu.driver_version = values[2];
                            
                            // Temperature
                            gpu.temperature_celsius = std::stod(values[4]);
                            
                            // Power
                            gpu.power_usage_watts = std::stod(values[5]);
                            gpu.max_power_watts = std::stod(values[6]);
                            
                            // Clock speeds
                            gpu.compute_info.core_clock_mhz = std::stod(values[7]);
                            gpu.compute_info.boost_clock_mhz = std::stod(values[8]);
                        } catch (...) {
                            // Ignore parsing errors
                        }
                        
                        break;
                    }
                }
            }
        }
    }
    
    return !gpu_info_.empty();
#else
    return false;
#endif
}

bool GPUDetector::detectCUDAGPUs() {
#if defined(HAVE_CUDA)
    // Initialize CUDA runtime
    cudaError_t cuda_error = cudaInit(0);
    if (cuda_error != cudaSuccess) {
        return false;
    }
    
    // Get number of CUDA devices
    int device_count = 0;
    cuda_error = cudaGetDeviceCount(&device_count);
    if (cuda_error != cudaSuccess || device_count == 0) {
        return false;
    }
    
    // Initialize NVML for more detailed information
    nvmlReturn_t nvml_result = nvmlInit();
    bool nvml_initialized = (nvml_result == NVML_SUCCESS);
    
    // Get information for each CUDA device
    for (int i = 0; i < device_count; ++i) {
        cudaDeviceProp prop;
        cuda_error = cudaGetDeviceProperties(&prop, i);
        if (cuda_error != cudaSuccess) {
            continue;
        }
        
        GPUInfo gpu;
        gpu.device_id = i;
        gpu.name = prop.name;
        gpu.vendor = GPUVendor::NVIDIA;
        
        // Add CUDA API support
        gpu.api_support.push_back(GPUAPISupport::CUDA);
        
        // Memory information
        gpu.memory_info.total_memory_bytes = prop.totalGlobalMem;
        gpu.memory_info.memory_bus_width = prop.memoryBusWidth;
        gpu.memory_info.memory_clock_mhz = prop.memoryClockRate / 1000.0;
        
        // Calculate memory bandwidth
        gpu.memory_info.memory_bandwidth_gbps = 2.0 * gpu.memory_info.memory_clock_mhz * (gpu.memory_info.memory_bus_width / 8) / 1000.0;
        
        // Compute information
        gpu.compute_info.cuda_cores = prop.multiProcessorCount * _ConvertSMVer2Cores(prop.major, prop.minor);
        gpu.compute_info.core_clock_mhz = prop.clockRate / 1000.0;
        gpu.compute_info.cuda_compute_capability = std::to_string(prop.major) + "." + std::to_string(prop.minor);
        
        // Determine architecture based on compute capability
        gpu.architecture = determineNVIDIAArchitecture(gpu.name, gpu.compute_info.cuda_compute_capability);
        
        // PCIe information
        gpu.pcie_generation = prop.pciDeviceID >> 16;
        gpu.pcie_lanes = prop.pciDeviceID & 0xFFFF;
        
        // Check if integrated
        gpu.is_integrated = (prop.integrated != 0);
        
        // Calculate theoretical performance
        double ops_per_cycle = gpu.compute_info.cuda_cores * 2; // FMA = 2 operations per cycle
        gpu.compute_info.theoretical_tflops_fp32 = ops_per_cycle * gpu.compute_info.core_clock_mhz / 1e6;
        
        // Get more detailed information from NVML if available
        if (nvml_initialized) {
            nvmlDevice_t device;
            nvml_result = nvmlDeviceGetHandleByIndex(i, &device);
            if (nvml_result == NVML_SUCCESS) {
                // Get temperature
                unsigned int temperature;
                nvml_result = nvmlDeviceGetTemperature(device, NVML_TEMPERATURE_GPU, &temperature);
                if (nvml_result == NVML_SUCCESS) {
                    gpu.temperature_celsius = temperature;
                }
                
                // Get power usage
                unsigned int power;
                nvml_result = nvmlDeviceGetPowerUsage(device, &power);
                if (nvml_result == NVML_SUCCESS) {
                    gpu.power_usage_watts = power / 1000.0;
                }
                
                // Get power limit
                unsigned int power_limit;
                nvml_result = nvmlDeviceGetPowerManagementLimit(device, &power_limit);
                if (nvml_result == NVML_SUCCESS) {
                    gpu.max_power_watts = power_limit / 1000.0;
                }
                
                // Get memory info
                nvmlMemory_t memory;
                nvml_result = nvmlDeviceGetMemoryInfo(device, &memory);
                if (nvml_result == NVML_SUCCESS) {
                    gpu.memory_info.total_memory_bytes = memory.total;
                    gpu.memory_info.free_memory_bytes = memory.free;
                    gpu.memory_info.used_memory_bytes = memory.used;
                }
                
                // Get ECC state
                nvmlEnableState_t ecc_current;
                nvml_result = nvmlDeviceGetEccMode(device, &ecc_current, NULL);
                if (nvml_result == NVML_SUCCESS) {
                    gpu.memory_info.ecc_enabled = (ecc_current == NVML_FEATURE_ENABLED);
                }
            }
        }
        
        // Add to list
        gpu_info_.push_back(gpu);
    }
    
    // Shutdown NVML if initialized
    if (nvml_initialized) {
        nvmlShutdown();
    }
    
    return !gpu_info_.empty();
#else
    // Try to detect CUDA GPUs using nvidia-smi
    std::string output;
    if (executeCommand("nvidia-smi --query-gpu=name,memory.total,driver_version,pci.bus_id,temperature.gpu,power.draw,power.limit,clocks.current.graphics,clocks.max.graphics --format=csv,noheader,nounits", output)) {
        std::istringstream iss(output);
        std::string line;
        
        int device_id = 0;
        while (std::getline(iss, line)) {
            std::vector<std::string> values;
            std::istringstream line_iss(line);
            std::string value;
            
            // Parse CSV line
            while (std::getline(line_iss, value, ',')) {
                // Trim whitespace
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);
                values.push_back(value);
            }
            
            if (values.size() >= 9) {
                GPUInfo gpu;
                gpu.device_id = device_id++;
                gpu.name = values[0];
                gpu.vendor = GPUVendor::NVIDIA;
                gpu.api_support.push_back(GPUAPISupport::CUDA);
                
                try {
                    // Memory in MiB
                    gpu.memory_info.total_memory_bytes = std::stoull(values[1]) * 1024 * 1024;
                    
                    // Driver version
                    gpu.driver_version = values[2];
                    
                    // Temperature
                    gpu.temperature_celsius = std::stod(values[4]);
                    
                    // Power
                    gpu.power_usage_watts = std::stod(values[5]);
                    gpu.max_power_watts = std::stod(values[6]);
                    
                    // Clock speeds
                    gpu.compute_info.core_clock_mhz = std::stod(values[7]);
                    gpu.compute_info.boost_clock_mhz = std::stod(values[8]);
                } catch (...) {
                    // Ignore parsing errors
                }
                
                // Determine architecture
                gpu.architecture = determineNVIDIAArchitecture(gpu.name, "");
                
                // Add to list
                gpu_info_.push_back(gpu);
            }
        }
        
        return !gpu_info_.empty();
    }
    
    return false;
#endif
}

bool GPUDetector::detectOpenCLGPUs() {
#if defined(HAVE_OPENCL)
    // Get OpenCL platforms
    cl_uint num_platforms;
    cl_int error = clGetPlatformIDs(0, NULL, &num_platforms);
    if (error != CL_SUCCESS || num_platforms == 0) {
        return false;
    }
    
    std::vector<cl_platform_id> platforms(num_platforms);
    error = clGetPlatformIDs(num_platforms, platforms.data(), NULL);
    if (error != CL_SUCCESS) {
        return false;
    }
    
    // For each platform, get devices
    for (cl_uint p = 0; p < num_platforms; ++p) {
        cl_platform_id platform = platforms[p];
        
        // Get platform name
        char platform_name[1024];
        error = clGetPlatformInfo(platform, CL_PLATFORM_NAME, sizeof(platform_name), platform_name, NULL);
        if (error != CL_SUCCESS) {
            continue;
        }
        
        // Get devices
        cl_uint num_devices;
        error = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, NULL, &num_devices);
        if (error != CL_SUCCESS || num_devices == 0) {
            continue;
        }
        
        std::vector<cl_device_id> devices(num_devices);
        error = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, num_devices, devices.data(), NULL);
        if (error != CL_SUCCESS) {
            continue;
        }
        
        // Get information for each device
        for (cl_uint d = 0; d < num_devices; ++d) {
            cl_device_id device = devices[d];
            
            GPUInfo gpu;
            gpu.device_id = static_cast<int>(gpu_info_.size());
            gpu.api_support.push_back(GPUAPISupport::OPENCL);
            
            // Get device name
            char device_name[1024];
            error = clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
            if (error == CL_SUCCESS) {
                gpu.name = device_name;
            }
            
            // Get device vendor
            char device_vendor[1024];
            error = clGetDeviceInfo(device, CL_DEVICE_VENDOR, sizeof(device_vendor), device_vendor, NULL);
            if (error == CL_SUCCESS) {
                std::string vendor_str = device_vendor;
                if (vendor_str.find("NVIDIA") != std::string::npos) {
                    gpu.vendor = GPUVendor::NVIDIA;
                } else if (vendor_str.find("AMD") != std::string::npos || vendor_str.find("Advanced Micro Devices") != std::string::npos) {
                    gpu.vendor = GPUVendor::AMD;
                } else if (vendor_str.find("Intel") != std::string::npos) {
                    gpu.vendor = GPUVendor::INTEL;
                } else if (vendor_str.find("Apple") != std::string::npos) {
                    gpu.vendor = GPUVendor::APPLE;
                }
            }
            
            // Get device version
            char device_version[1024];
            error = clGetDeviceInfo(device, CL_DEVICE_VERSION, sizeof(device_version), device_version, NULL);
            if (error == CL_SUCCESS) {
                gpu.driver_version = device_version;
                
                // Extract OpenCL version
                std::string version_str = device_version;
                std::regex version_regex("OpenCL (\\d+)\\.(\\d+)");
                std::smatch match;
                if (std::regex_search(version_str, match, version_regex) && match.size() > 2) {
                    try {
                        gpu.compute_info.opencl_version_major = std::stoi(match[1].str());
                        gpu.compute_info.opencl_version_minor = std::stoi(match[2].str());
                    } catch (...) {
                        // Ignore parsing errors
                    }
                }
            }
            
            // Get global memory size
            cl_ulong global_mem_size;
            error = clGetDeviceInfo(device, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(global_mem_size), &global_mem_size, NULL);
            if (error == CL_SUCCESS) {
                gpu.memory_info.total_memory_bytes = global_mem_size;
            }
            
            // Get compute units
            cl_uint compute_units;
            error = clGetDeviceInfo(device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(compute_units), &compute_units, NULL);
            if (error == CL_SUCCESS) {
                gpu.compute_info.compute_units = compute_units;
            }
            
            // Get clock frequency
            cl_uint clock_frequency;
            error = clGetDeviceInfo(device, CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(clock_frequency), &clock_frequency, NULL);
            if (error == CL_SUCCESS) {
                gpu.compute_info.core_clock_mhz = clock_frequency;
            }
            
            // Check if device is integrated
            cl_bool is_integrated;
            error = clGetDeviceInfo(device, CL_DEVICE_HOST_UNIFIED_MEMORY, sizeof(is_integrated), &is_integrated, NULL);
            if (error == CL_SUCCESS) {
                gpu.is_integrated = (is_integrated == CL_TRUE);
            }
            
            // Determine architecture
            if (gpu.vendor == GPUVendor::NVIDIA) {
                gpu.architecture = determineNVIDIAArchitecture(gpu.name, "");
            } else if (gpu.vendor == GPUVendor::AMD) {
                gpu.architecture = determineAMDArchitecture(gpu.name);
            } else if (gpu.vendor == GPUVendor::INTEL) {
                gpu.architecture = determineIntelArchitecture(gpu.name);
            } else if (gpu.vendor == GPUVendor::APPLE) {
                gpu.architecture = determineAppleArchitecture(gpu.name);
            }
            
            // Calculate theoretical performance
            if (gpu.compute_info.compute_units > 0 && gpu.compute_info.core_clock_mhz > 0) {
                // Rough estimate: compute_units * 64 (work items per compute unit) * 2 (FMA) * clock_frequency
                double ops_per_cycle = gpu.compute_info.compute_units * 64 * 2;
                gpu.compute_info.theoretical_tflops_fp32 = ops_per_cycle * gpu.compute_info.core_clock_mhz / 1e6;
            }
            
            // Add to list
            gpu_info_.push_back(gpu);
        }
    }
    
    return !gpu_info_.empty();
#else
    // Try to detect OpenCL GPUs using clinfo
    std::string output;
    if (executeCommand("clinfo --raw", output)) {
        std::istringstream iss(output);
        std::string line;
        
        GPUInfo current_gpu;
        bool has_gpu = false;
        
        while (std::getline(iss, line)) {
            if (line.find("Device Name") != std::string::npos) {
                if (has_gpu) {
                    // Add previous GPU to list
                    current_gpu.device_id = static_cast<int>(gpu_info_.size());
                    gpu_info_.push_back(current_gpu);
                    current_gpu = GPUInfo();
                }
                
                size_t pos = line.find(':');
                if (pos != std::string::npos) {
                    current_gpu.name = line.substr(pos + 1);
                    // Trim whitespace
                    current_gpu.name.erase(0, current_gpu.name.find_first_not_of(" \t"));
                    current_gpu.name.erase(current_gpu.name.find_last_not_of(" \t") + 1);
                    has_gpu = true;
                    
                    // Add OpenCL API support
                    current_gpu.api_support.push_back(GPUAPISupport::OPENCL);
                }
            } else if (has_gpu) {
                if (line.find("Device Vendor") != std::string::npos) {
                    size_t pos = line.find(':');
                    if (pos != std::string::npos) {
                        std::string vendor = line.substr(pos + 1);
                        // Trim whitespace
                        vendor.erase(0, vendor.find_first_not_of(" \t"));
                        vendor.erase(vendor.find_last_not_of(" \t") + 1);
                        
                        if (vendor.find("NVIDIA") != std::string::npos) {
                            current_gpu.vendor = GPUVendor::NVIDIA;
                        } else if (vendor.find("AMD") != std::string::npos || vendor.find("Advanced Micro Devices") != std::string::npos) {
                            current_gpu.vendor = GPUVendor::AMD;
                        } else if (vendor.find("Intel") != std::string::npos) {
                            current_gpu.vendor = GPUVendor::INTEL;
                        } else if (vendor.find("Apple") != std::string::npos) {
                            current_gpu.vendor = GPUVendor::APPLE;
                        }
                    }
                } else if (line.find("Device Version") != std::string::npos) {
                    size_t pos = line.find(':');
                    if (pos != std::string::npos) {
                        current_gpu.driver_version = line.substr(pos + 1);
                        // Trim whitespace
                        current_gpu.driver_version.erase(0, current_gpu.driver_version.find_first_not_of(" \t"));
                        current_gpu.driver_version.erase(current_gpu.driver_version.find_last_not_of(" \t") + 1);
                    }
                } else if (line.find("Global Memory Size") != std::string::npos) {
                    size_t pos = line.find(':');
                    if (pos != std::string::npos) {
                        std::string mem_str = line.substr(pos + 1);
                        // Trim whitespace
                        mem_str.erase(0, mem_str.find_first_not_of(" \t"));
                        mem_str.erase(mem_str.find_last_not_of(" \t") + 1);
                        
                        // Extract numeric part
                        std::string mem_value;
                        for (char c : mem_str) {
                            if (std::isdigit(c)) {
                                mem_value += c;
                            } else if (!mem_value.empty()) {
                                break;
                            }
                        }
                        
                        try {
                            current_gpu.memory_info.total_memory_bytes = std::stoull(mem_value);
                        } catch (...) {
                            // Ignore parsing errors
                        }
                    }
                } else if (line.find("Max Compute Units") != std::string::npos) {
                    size_t pos = line.find(':');
                    if (pos != std::string::npos) {
                        std::string cu_str = line.substr(pos + 1);
                        // Trim whitespace
                        cu_str.erase(0, cu_str.find_first_not_of(" \t"));
                        cu_str.erase(cu_str.find_last_not_of(" \t") + 1);
                        
                        try {
                            current_gpu.compute_info.compute_units = std::stoi(cu_str);
                        } catch (...) {
                            // Ignore parsing errors
                        }
                    }
                } else if (line.find("Max Clock Frequency") != std::string::npos) {
                    size_t pos = line.find(':');
                    if (pos != std::string::npos) {
                        std::string freq_str = line.substr(pos + 1);
                        // Trim whitespace
                        freq_str.erase(0, freq_str.find_first_not_of(" \t"));
                        freq_str.erase(freq_str.find_last_not_of(" \t") + 1);
                        
                        try {
                            current_gpu.compute_info.core_clock_mhz = std::stod(freq_str);
                        } catch (...) {
                            // Ignore parsing errors
                        }
                    }
                }
            }
        }
        
        // Add last GPU if any
        if (has_gpu) {
            current_gpu.device_id = static_cast<int>(gpu_info_.size());
            gpu_info_.push_back(current_gpu);
        }
        
        return !gpu_info_.empty();
    }
    
    return false;
#endif
}

bool GPUDetector::detectMetalGPUs() {
#if defined(__APPLE__) && defined(HAVE_METAL)
    // Get Metal devices
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (!device) {
        return false;
    }
    
    NSArray<id<MTLDevice>> *devices = MTLCopyAllDevices();
    if (!devices || [devices count] == 0) {
        return false;
    }
    
    for (NSUInteger i = 0; i < [devices count]; i++) {
        id<MTLDevice> device = devices[i];
        
        GPUInfo gpu;
        gpu.device_id = static_cast<int>(gpu_info_.size());
        gpu.api_support.push_back(GPUAPISupport::METAL);
        
        // Get device name
        gpu.name = [[device name] UTF8String];
        
        // Determine vendor and architecture
        if ([device isLowPower]) {
            // Integrated GPU
            gpu.is_integrated = true;
            
            if (gpu.name.find("Apple") != std::string::npos) {
                gpu.vendor = GPUVendor::APPLE;
                gpu.architecture = determineAppleArchitecture(gpu.name);
            } else if (gpu.name.find("Intel") != std::string::npos) {
                gpu.vendor = GPUVendor::INTEL;
                gpu.architecture = determineIntelArchitecture(gpu.name);
            }
        } else {
            // Discrete GPU
            gpu.is_integrated = false;
            
            if (gpu.name.find("AMD") != std::string::npos || gpu.name.find("Radeon") != std::string::npos) {
                gpu.vendor = GPUVendor::AMD;
                gpu.architecture = determineAMDArchitecture(gpu.name);
            } else if (gpu.name.find("NVIDIA") != std::string::npos) {
                gpu.vendor = GPUVendor::NVIDIA;
                gpu.architecture = determineNVIDIAArchitecture(gpu.name, "");
            } else if (gpu.name.find("Apple") != std::string::npos) {
                gpu.vendor = GPUVendor::APPLE;
                gpu.architecture = determineAppleArchitecture(gpu.name);
            }
        }
        
        // Get memory information
        if (@available(macOS 10.13, *)) {
            gpu.memory_info.total_memory_bytes = [device recommendedMaxWorkingSetSize];
        }
        
        // Add to list
        gpu_info_.push_back(gpu);
    }
    
    return !gpu_info_.empty();
#else
    // On macOS, we can use system_profiler to get GPU information
    #ifdef __APPLE__
    std::string output;
    if (executeCommand("system_profiler SPDisplaysDataType", output)) {
        std::istringstream iss(output);
        std::string line;
        
        GPUInfo current_gpu;
        bool has_gpu = false;
        
        while (std::getline(iss, line)) {
            // Check for GPU name
            if (line.find("Chipset Model:") != std::string::npos) {
                if (has_gpu) {
                    // Add previous GPU to list
                    current_gpu.device_id = static_cast<int>(gpu_info_.size());
                    gpu_info_.push_back(current_gpu);
                    current_gpu = GPUInfo();
                }
                
                size_t pos = line.find(':');
                if (pos != std::string::npos) {
                    current_gpu.name = line.substr(pos + 1);
                    // Trim whitespace
                    current_gpu.name.erase(0, current_gpu.name.find_first_not_of(" \t"));
                    current_gpu.name.erase(current_gpu.name.find_last_not_of(" \t") + 1);
                    has_gpu = true;
                    
                    // Determine vendor and architecture
                    if (current_gpu.name.find("Apple") != std::string::npos) {
                        current_gpu.vendor = GPUVendor::APPLE;
                        current_gpu.architecture = determineAppleArchitecture(current_gpu.name);
                        current_gpu.api_support.push_back(GPUAPISupport::METAL);
                        current_gpu.is_integrated = true;
                    } else if (current_gpu.name.find("NVIDIA") != std::string::npos) {
                        current_gpu.vendor = GPUVendor::NVIDIA;
                        current_gpu.architecture = determineNVIDIAArchitecture(current_gpu.name, "");
                        current_gpu.api_support.push_back(GPUAPISupport::METAL);
                    } else if (current_gpu.name.find("AMD") != std::string::npos || 
                              current_gpu.name.find("Radeon") != std::string::npos) {
                        current_gpu.vendor = GPUVendor::AMD;
                        current_gpu.architecture = determineAMDArchitecture(current_gpu.name);
                        current_gpu.api_support.push_back(GPUAPISupport::METAL);
                    } else if (current_gpu.name.find("Intel") != std::string::npos) {
                        current_gpu.vendor = GPUVendor::INTEL;
                        current_gpu.architecture = determineIntelArchitecture(current_gpu.name);
                        current_gpu.api_support.push_back(GPUAPISupport::METAL);
                        current_gpu.is_integrated = true;
                    }
                }
            } else if (has_gpu) {
                // Check for memory information
                if (line.find("VRAM") != std::string::npos) {
                    size_t pos = line.find(':');
                    if (pos != std::string::npos) {
                        std::string vram_str = line.substr(pos + 1);
                        // Trim whitespace
                        vram_str.erase(0, vram_str.find_first_not_of(" \t"));
                        vram_str.erase(vram_str.find_last_not_of(" \t") + 1);
                        
                        // Extract numeric part
                        std::string vram_value;
                        for (char c : vram_str) {
                            if (std::isdigit(c) || c == '.') {
                                vram_value += c;
                            } else if (!vram_value.empty()) {
                                break;
                            }
                        }
                        
                        try {
                            double vram_gb = std::stod(vram_value);
                            current_gpu.memory_info.total_memory_bytes = static_cast<size_t>(vram_gb * 1024 * 1024 * 1024);
                        } catch (...) {
                            // Ignore parsing errors
                        }
                    }
                }
            }
        }
        
        // Add last GPU if any
        if (has_gpu) {
            current_gpu.device_id = static_cast<int>(gpu_info_.size());
            gpu_info_.push_back(current_gpu);
        }
        
        return !gpu_info_.empty();
    }
    #endif
    
    return false;
#endif
}

bool GPUDetector::detectVulkanGPUs() {
    // Try to detect Vulkan GPUs using vulkaninfo
    std::string output;
    if (executeCommand("vulkaninfo --summary", output)) {
        std::istringstream iss(output);
        std::string line;
        
        GPUInfo current_gpu;
        bool has_gpu = false;
        
        while (std::getline(iss, line)) {
            if (line.find("GPU") != std::string::npos && line.find("name") != std::string::npos) {
                if (has_gpu) {
                    // Add previous GPU to list
                    current_gpu.device_id = static_cast<int>(gpu_info_.size());
                    gpu_info_.push_back(current_gpu);
                    current_gpu = GPUInfo();
                }
                
                size_t pos = line.find(':');
                if (pos != std::string::npos) {
                    current_gpu.name = line.substr(pos + 1);
                    // Trim whitespace
                    current_gpu.name.erase(0, current_gpu.name.find_first_not_of(" \t"));
                    current_gpu.name.erase(current_gpu.name.find_last_not_of(" \t") + 1);
                    has_gpu = true;
                    
                    // Add Vulkan API support
                    current_gpu.api_support.push_back(GPUAPISupport::VULKAN);
                    
                    // Determine vendor and architecture
                    if (current_gpu.name.find("NVIDIA") != std::string::npos) {
                        current_gpu.vendor = GPUVendor::NVIDIA;
                        current_gpu.architecture = determineNVIDIAArchitecture(current_gpu.name, "");
                    } else if (current_gpu.name.find("AMD") != std::string::npos || 
                              current_gpu.name.find("Radeon") != std::string::npos) {
                        current_gpu.vendor = GPUVendor::AMD;
                        current_gpu.architecture = determineAMDArchitecture(current_gpu.name);
                    } else if (current_gpu.name.find("Intel") != std::string::npos) {
                        current_gpu.vendor = GPUVendor::INTEL;
                        current_gpu.architecture = determineIntelArchitecture(current_gpu.name);
                        current_gpu.is_integrated = true;
                    } else if (current_gpu.name.find("Apple") != std::string::npos) {
                        current_gpu.vendor = GPUVendor::APPLE;
                        current_gpu.architecture = determineAppleArchitecture(current_gpu.name);
                    }
                }
            } else if (has_gpu) {
                // Check for driver version
                if (line.find("driverVersion") != std::string::npos) {
                    size_t pos = line.find(':');
                    if (pos != std::string::npos) {
                        current_gpu.driver_version = line.substr(pos + 1);
                        // Trim whitespace
                        current_gpu.driver_version.erase(0, current_gpu.driver_version.find_first_not_of(" \t"));
                        current_gpu.driver_version.erase(current_gpu.driver_version.find_last_not_of(" \t") + 1);
                    }
                }
            }
        }
        
        // Add last GPU if any
        if (has_gpu) {
            current_gpu.device_id = static_cast<int>(gpu_info_.size());
            gpu_info_.push_back(current_gpu);
        }
        
        return !gpu_info_.empty();
    }
    
    return false;
}

bool GPUDetector::detectDirectXGPUs() {
#ifdef _WIN32
    // Try to detect DirectX GPUs using dxdiag
    std::string output;
    if (executeCommand("dxdiag /t dxdiag_output.txt", output)) {
        // Wait for dxdiag to finish
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        // Read dxdiag output
        std::ifstream file("dxdiag_output.txt");
        if (file.is_open()) {
            std::string line;
            
            GPUInfo current_gpu;
            bool has_gpu = false;
            bool in_display_section = false;
            
            while (std::getline(file, line)) {
                // Check for display section
                if (line.find("-------------") != std::string::npos) {
                    if (has_gpu) {
                        // Add previous GPU to list
                        current_gpu.device_id = static_cast<int>(gpu_info_.size());
                        gpu_info_.push_back(current_gpu);
                        current_gpu = GPUInfo();
                        has_gpu = false;
                    }
                    
                    in_display_section = false;
                } else if (line.find("Display Devices") != std::string::npos) {
                    in_display_section = true;
                } else if (in_display_section) {
                    // Check for GPU name
                    if (line.find("Card name:") != std::string::npos) {
                        size_t pos = line.find(':');
                        if (pos != std::string::npos) {
                            current_gpu.name = line.substr(pos + 1);
                            // Trim whitespace
                            current_gpu.name.erase(0, current_gpu.name.find_first_not_of(" \t"));
                            current_gpu.name.erase(current_gpu.name.find_last_not_of(" \t") + 1);
                            has_gpu = true;
                            
                            // Add DirectX API support
                            current_gpu.api_support.push_back(GPUAPISupport::DIRECTX);
                            
                            // Determine vendor and architecture
                            if (current_gpu.name.find("NVIDIA") != std::string::npos) {
                                current_gpu.vendor = GPUVendor::NVIDIA;
                                current_gpu.architecture = determineNVIDIAArchitecture(current_gpu.name, "");
                            } else if (current_gpu.name.find("AMD") != std::string::npos || 
                                      current_gpu.name.find("Radeon") != std::string::npos) {
                                current_gpu.vendor = GPUVendor::AMD;
                                current_gpu.architecture = determineAMDArchitecture(current_gpu.name);
                            } else if (current_gpu.name.find("Intel") != std::string::npos) {
                                current_gpu.vendor = GPUVendor::INTEL;
                                current_gpu.architecture = determineIntelArchitecture(current_gpu.name);
                                current_gpu.is_integrated = true;
                            }
                        }
                    } else if (has_gpu) {
                        // Check for driver version
                        if (line.find("Driver Version:") != std::string::npos) {
                            size_t pos = line.find(':');
                            if (pos != std::string::npos) {
                                current_gpu.driver_version = line.substr(pos + 1);
                                // Trim whitespace
                                current_gpu.driver_version.erase(0, current_gpu.driver_version.find_first_not_of(" \t"));
                                current_gpu.driver_version.erase(current_gpu.driver_version.find_last_not_of(" \t") + 1);
                            }
                        } else if (line.find("Dedicated Memory:") != std::string::npos) {
                            size_t pos = line.find(':');
                            if (pos != std::string::npos) {
                                std::string mem_str = line.substr(pos + 1);
                                // Trim whitespace
                                mem_str.erase(0, mem_str.find_first_not_of(" \t"));
                                mem_str.erase(mem_str.find_last_not_of(" \t") + 1);
                                
                                // Extract numeric part
                                std::regex mem_regex("(\\d+)\\s*([KMG]B)");
                                std::smatch match;
                                if (std::regex_search(mem_str, match, mem_regex) && match.size() > 2) {
                                    try {
                                        size_t mem_size = std::stoull(match[1].str());
                                        std::string unit = match[2].str();
                                        
                                        if (unit == "KB") {
                                            mem_size *= 1024;
                                        } else if (unit == "MB") {
                                            mem_size *= 1024 * 1024;
                                        } else if (unit == "GB") {
                                            mem_size *= 1024 * 1024 * 1024;
                                        }
                                        
                                        current_gpu.memory_info.total_memory_bytes = mem_size;
                                    } catch (...) {
                                        // Ignore parsing errors
                                    }
                                }
                            }
                        }
                    }
                }
            }
            
            // Add last GPU if any
            if (has_gpu) {
                current_gpu.device_id = static_cast<int>(gpu_info_.size());
                gpu_info_.push_back(current_gpu);
            }
            
            // Clean up
            file.close();
            std::remove("dxdiag_output.txt");
            
            return !gpu_info_.empty();
        }
    }
#endif
    
    return false;
}

bool GPUDetector::executeCommand(const std::string& command, std::string& output) {
    output.clear();
    
#if defined(_WIN32)
    // Windows
    HANDLE hReadPipe, hWritePipe;
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;
    
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
        return false;
    }
    
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(STARTUPINFO));
    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
    si.cb = sizeof(STARTUPINFO);
    si.hStdError = hWritePipe;
    si.hStdOutput = hWritePipe;
    si.dwFlags |= STARTF_USESTDHANDLES;
    
    if (!CreateProcess(NULL, const_cast<LPSTR>(command.c_str()), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return false;
    }
    
    CloseHandle(hWritePipe);
    
    char buffer[4096];
    DWORD bytesRead;
    while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        output += buffer;
    }
    
    CloseHandle(hReadPipe);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
    return true;
#else
    // Unix-like systems
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        return false;
    }
    
    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        output += buffer;
    }
    
    int status = pclose(pipe);
    return status == 0;
#endif
}

GPUArchitecture GPUDetector::determineNVIDIAArchitecture(const std::string& device_name, const std::string& compute_capability) {
    // Determine architecture based on compute capability
    if (!compute_capability.empty()) {
        int major = std::stoi(compute_capability.substr(0, 1));
        int minor = std::stoi(compute_capability.substr(2, 1));
        
        if (major == 3) {
            return GPUArchitecture::KEPLER;
        } else if (major == 5) {
            return GPUArchitecture::MAXWELL;
        } else if (major == 6) {
            return GPUArchitecture::PASCAL;
        } else if (major == 7) {
            if (minor == 0) {
                return GPUArchitecture::VOLTA;
            } else {
                return GPUArchitecture::TURING;
            }
        } else if (major == 8) {
            return GPUArchitecture::AMPERE;
        } else if (major == 9) {
            if (minor == 0) {
                return GPUArchitecture::ADA_LOVELACE;
            } else {
                return GPUArchitecture::HOPPER;
            }
        }
    }
    
    // Determine architecture based on device name
    if (device_name.find("RTX 40") != std::string::npos || 
        device_name.find("RTX Ada") != std::string::npos ||
        device_name.find("AD10") != std::string::npos) {
        return GPUArchitecture::ADA_LOVELACE;
    } else if (device_name.find("RTX 30") != std::string::npos || 
               device_name.find("RTX A") != std::string::npos ||
               device_name.find("A100") != std::string::npos ||
               device_name.find("GA10") != std::string::npos) {
        return GPUArchitecture::AMPERE;
    } else if (device_name.find("RTX 20") != std::string::npos || 
               device_name.find("GTX 16") != std::string::npos ||
               device_name.find("TU10") != std::string::npos) {
        return GPUArchitecture::TURING;
    } else if (device_name.find("V100") != std::string::npos ||
               device_name.find("GV10") != std::string::npos) {
        return GPUArchitecture::VOLTA;
    } else if (device_name.find("GTX 10") != std::string::npos ||
               device_name.find("GP10") != std::string::npos) {
        return GPUArchitecture::PASCAL;
    } else if (device_name.find("GTX 9") != std::string::npos ||
               device_name.find("GM10") != std::string::npos) {
        return GPUArchitecture::MAXWELL;
    } else if (device_name.find("GTX 7") != std::string::npos ||
               device_name.find("GK10") != std::string::npos) {
        return GPUArchitecture::KEPLER;
    }
    
    return GPUArchitecture::UNKNOWN;
}

GPUArchitecture GPUDetector::determineAMDArchitecture(const std::string& device_name) {
    // Determine architecture based on device name
    if (device_name.find("RX 7") != std::string::npos ||
        device_name.find("RDNA 3") != std::string::npos ||
        device_name.find("Navi 3") != std::string::npos) {
        return GPUArchitecture::RDNA3;
    } else if (device_name.find("RX 6") != std::string::npos ||
               device_name.find("RDNA 2") != std::string::npos ||
               device_name.find("Navi 2") != std::string::npos) {
        return GPUArchitecture::RDNA2;
    } else if (device_name.find("RX 5") != std::string::npos ||
               device_name.find("RDNA") != std::string::npos ||
               device_name.find("Navi 1") != std::string::npos) {
        return GPUArchitecture::RDNA;
    } else if (device_name.find("Vega") != std::string::npos ||
               device_name.find("RX 5") != std::string::npos ||
               device_name.find("RX 4") != std::string::npos ||
               device_name.find("R9") != std::string::npos ||
               device_name.find("R7") != std::string::npos) {
        return GPUArchitecture::GCN;
    }
    
    return GPUArchitecture::UNKNOWN;
}

GPUArchitecture GPUDetector::determineIntelArchitecture(const std::string& device_name) {
    // Determine architecture based on device name
    if (device_name.find("Arc") != std::string::npos) {
        return GPUArchitecture::ARC;
    } else if (device_name.find("Xe") != std::string::npos) {
        return GPUArchitecture::XE;
    } else if (device_name.find("Gen11") != std::string::npos ||
               device_name.find("Iris Plus") != std::string::npos) {
        return GPUArchitecture::GEN11;
    } else if (device_name.find("Gen9") != std::string::npos ||
               device_name.find("HD Graphics") != std::string::npos ||
               device_name.find("UHD Graphics") != std::string::npos) {
        return GPUArchitecture::GEN9;
    }
    
    return GPUArchitecture::UNKNOWN;
}

GPUArchitecture GPUDetector::determineAppleArchitecture(const std::string& device_name) {
    // Determine architecture based on device name
    if (device_name.find("M3") != std::string::npos) {
        return GPUArchitecture::M3;
    } else if (device_name.find("M2") != std::string::npos) {
        return GPUArchitecture::M2;
    } else if (device_name.find("M1") != std::string::npos) {
        return GPUArchitecture::M1;
    }
    
    return GPUArchitecture::UNKNOWN;
}

} // namespace system
} // namespace mfp
