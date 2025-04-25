#include "hardware/gpu_detector.h"
#include <sstream>
#include <iostream>
#include <regex>
#include <algorithm>
#include <cstdio>
#include <memory>
#include <array>

#ifdef __linux__
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#elif defined(__APPLE__)
#include <sys/sysctl.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

namespace mfp {

GPUInfo::GPUInfo() {
    // Initialize with defaults
}

GPUInfo::~GPUInfo() {
    // Nothing to clean up
}

std::string GPUInfo::getSummary() const {
    std::stringstream ss;
    
    ss << "GPU Information:" << std::endl;
    ss << "  Name: " << m_name << std::endl;
    
    ss << "  Vendor: ";
    switch (m_vendor) {
        case GPUVendor::NVIDIA: ss << "NVIDIA"; break;
        case GPUVendor::AMD: ss << "AMD"; break;
        case GPUVendor::INTEL: ss << "Intel"; break;
        case GPUVendor::APPLE: ss << "Apple"; break;
        default: ss << "Unknown"; break;
    }
    ss << std::endl;
    
    ss << "  Driver Version: " << m_driver_version << std::endl;
    ss << "  Integrated: " << (m_is_integrated ? "Yes" : "No") << std::endl;
    
    ss << "  APIs: ";
    if (m_apis.supports_cuda) ss << "CUDA ";
    if (m_apis.supports_opencl) ss << "OpenCL ";
    if (m_apis.supports_metal) ss << "Metal ";
    if (m_apis.supports_directx) ss << "DirectX ";
    if (m_apis.supports_vulkan) ss << "Vulkan ";
    ss << std::endl;
    
    ss << "  Memory: " << (m_memory.total_memory_bytes / (1024 * 1024 * 1024)) << " GB";
    if (m_memory.memory_bandwidth_gbps > 0) {
        ss << ", " << m_memory.memory_bandwidth_gbps << " GB/s bandwidth";
    }
    ss << std::endl;
    
    ss << "  Compute: ";
    if (m_compute.compute_units > 0) {
        ss << m_compute.compute_units << " compute units";
    }
    if (m_compute.cuda_cores > 0) {
        ss << ", " << m_compute.cuda_cores << " CUDA cores";
    }
    if (m_compute.tensor_cores > 0) {
        ss << ", " << m_compute.tensor_cores << " Tensor cores";
    }
    if (m_compute.clock_mhz > 0) {
        ss << ", " << m_compute.clock_mhz << " MHz";
    }
    ss << std::endl;
    
    if (m_compute.tflops_fp32 > 0 || m_compute.tflops_fp16 > 0) {
        ss << "  Performance: ";
        if (m_compute.tflops_fp32 > 0) {
            ss << m_compute.tflops_fp32 << " TFLOPS (FP32)";
        }
        if (m_compute.tflops_fp16 > 0) {
            if (m_compute.tflops_fp32 > 0) ss << ", ";
            ss << m_compute.tflops_fp16 << " TFLOPS (FP16)";
        }
        ss << std::endl;
    }
    
    if (m_apis.supports_cuda && m_compute.cuda_compute_capability_major > 0) {
        ss << "  CUDA Compute Capability: " 
           << m_compute.cuda_compute_capability_major 
           << "." 
           << m_compute.cuda_compute_capability_minor 
           << std::endl;
    }
    
    return ss.str();
}

GPUDetector::GPUDetector() {
    // Initialize with defaults
}

GPUDetector::~GPUDetector() {
    // Nothing to clean up
}

void GPUDetector::detect() {
    // Try to detect GPUs using various APIs
    detectCUDAGPUs();
    detectOpenCLGPUs();
    detectMetalGPUs();
    detectDirectXGPUs();
    detectVulkanGPUs();
    
    // If no GPUs were detected, try using command-line tools
    if (m_gpus.empty()) {
        detectUsingCommandLine();
    }
    
    // Merge duplicate GPU entries and calculate performance metrics
    mergeGPUInfo();
    calculatePerformanceMetrics();
}

bool GPUDetector::hasCUDAGPU() const {
    for (const auto& gpu : m_gpus) {
        if (gpu.getAPIs().supports_cuda) {
            return true;
        }
    }
    return false;
}

bool GPUDetector::hasMetalGPU() const {
    for (const auto& gpu : m_gpus) {
        if (gpu.getAPIs().supports_metal) {
            return true;
        }
    }
    return false;
}

GPUInfo GPUDetector::findBestComputeGPU() const {
    if (m_gpus.empty()) {
        return GPUInfo();  // Return empty GPU info
    }
    
    // First, look for NVIDIA GPUs with CUDA support
    for (const auto& gpu : m_gpus) {
        if (gpu.getVendor() == GPUVendor::NVIDIA && gpu.getAPIs().supports_cuda && !gpu.isIntegrated()) {
            return gpu;
        }
    }
    
    // Then, look for AMD GPUs with OpenCL support
    for (const auto& gpu : m_gpus) {
        if (gpu.getVendor() == GPUVendor::AMD && gpu.getAPIs().supports_opencl && !gpu.isIntegrated()) {
            return gpu;
        }
    }
    
    // Then, look for Apple GPUs with Metal support
    for (const auto& gpu : m_gpus) {
        if (gpu.getVendor() == GPUVendor::APPLE && gpu.getAPIs().supports_metal) {
            return gpu;
        }
    }
    
    // Finally, just return the first GPU
    return m_gpus[0];
}

std::string GPUDetector::getSummary() const {
    std::stringstream ss;
    
    if (m_gpus.empty()) {
        ss << "No GPUs detected." << std::endl;
    } else {
        ss << "Detected " << m_gpus.size() << " GPU(s):" << std::endl;
        for (size_t i = 0; i < m_gpus.size(); i++) {
            ss << "GPU " << i << ":" << std::endl;
            ss << m_gpus[i].getSummary() << std::endl;
        }
    }
    
    return ss.str();
}

void GPUDetector::detectCUDAGPUs() {
    // This would normally use the CUDA API to detect NVIDIA GPUs
    // For simplicity, we'll use a command-line approach here
    
#ifdef __linux__
    // Try using nvidia-smi on Linux
    FILE* pipe = popen("nvidia-smi --query-gpu=name,driver_version,memory.total,memory.free,count,compute_cap --format=csv,noheader", "r");
    if (!pipe) {
        return;
    }
    
    char buffer[1024];
    std::string result = "";
    while (!feof(pipe)) {
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
    }
    pclose(pipe);
    
    // Parse the output
    std::istringstream iss(result);
    std::string line;
    while (std::getline(iss, line)) {
        // Format: name, driver_version, memory.total [MiB], memory.free [MiB], count, compute_cap
        std::regex pattern("([^,]+), ([^,]+), ([0-9]+) MiB, ([0-9]+) MiB, ([0-9]+), ([0-9]+\\.[0-9]+)");
        std::smatch matches;
        
        if (std::regex_search(line, matches, pattern) && matches.size() > 6) {
            GPUInfo gpu;
            gpu.setVendor(GPUVendor::NVIDIA);
            gpu.setName(matches[1].str());
            gpu.setDriverVersion(matches[2].str());
            
            GPUMemory memory;
            memory.total_memory_bytes = std::stoull(matches[3].str()) * 1024 * 1024;
            memory.available_memory_bytes = std::stoull(matches[4].str()) * 1024 * 1024;
            gpu.setMemory(memory);
            
            GPUCompute compute;
            std::string compute_cap = matches[6].str();
            size_t dot_pos = compute_cap.find('.');
            if (dot_pos != std::string::npos) {
                compute.cuda_compute_capability_major = std::stoi(compute_cap.substr(0, dot_pos));
                compute.cuda_compute_capability_minor = std::stoi(compute_cap.substr(dot_pos + 1));
            }
            gpu.setCompute(compute);
            
            GPUAPIs apis;
            apis.supports_cuda = true;
            gpu.setAPIs(apis);
            
            m_gpus.push_back(gpu);
        }
    }
#elif defined(_WIN32)
    // On Windows, we could use NVML or other approaches
    // For simplicity, we'll just check if the CUDA runtime DLL exists
    HMODULE cudaModule = LoadLibraryA("cudart64_*.dll");
    if (cudaModule != NULL) {
        // CUDA runtime is available, assume there's at least one NVIDIA GPU
        FreeLibrary(cudaModule);
        
        GPUInfo gpu;
        gpu.setVendor(GPUVendor::NVIDIA);
        gpu.setName("NVIDIA GPU");
        
        GPUAPIs apis;
        apis.supports_cuda = true;
        gpu.setAPIs(apis);
        
        m_gpus.push_back(gpu);
    }
#endif
}

void GPUDetector::detectOpenCLGPUs() {
    // This would normally use the OpenCL API to detect GPUs
    // For simplicity, we'll use a command-line approach here
    
#ifdef __linux__
    // Try using clinfo on Linux
    FILE* pipe = popen("clinfo --raw", "r");
    if (!pipe) {
        return;
    }
    
    char buffer[1024];
    std::string result = "";
    while (!feof(pipe)) {
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
    }
    pclose(pipe);
    
    // Parse the output (simplified)
    std::istringstream iss(result);
    std::string line;
    GPUInfo current_gpu;
    bool in_device_section = false;
    
    while (std::getline(iss, line)) {
        if (line.find("Device Type") != std::string::npos && line.find("GPU") != std::string::npos) {
            // Start of a GPU device section
            if (in_device_section) {
                // Save the previous GPU
                GPUAPIs apis = current_gpu.getAPIs();
                apis.supports_opencl = true;
                current_gpu.setAPIs(apis);
                m_gpus.push_back(current_gpu);
            }
            
            in_device_section = true;
            current_gpu = GPUInfo();
        } else if (in_device_section) {
            if (line.find("Device Name") != std::string::npos) {
                size_t pos = line.find(':');
                if (pos != std::string::npos) {
                    current_gpu.setName(line.substr(pos + 1));
                }
            } else if (line.find("Device Vendor") != std::string::npos) {
                if (line.find("NVIDIA") != std::string::npos) {
                    current_gpu.setVendor(GPUVendor::NVIDIA);
                } else if (line.find("AMD") != std::string::npos || line.find("Advanced Micro Devices") != std::string::npos) {
                    current_gpu.setVendor(GPUVendor::AMD);
                } else if (line.find("Intel") != std::string::npos) {
                    current_gpu.setVendor(GPUVendor::INTEL);
                } else if (line.find("Apple") != std::string::npos) {
                    current_gpu.setVendor(GPUVendor::APPLE);
                }
            } else if (line.find("Device Version") != std::string::npos) {
                size_t pos = line.find(':');
                if (pos != std::string::npos) {
                    current_gpu.setDriverVersion(line.substr(pos + 1));
                }
            } else if (line.find("Global Memory Size") != std::string::npos) {
                size_t pos = line.find(':');
                if (pos != std::string::npos) {
                    std::string mem_str = line.substr(pos + 1);
                    std::regex mem_regex("([0-9]+)");
                    std::smatch mem_match;
                    if (std::regex_search(mem_str, mem_match, mem_regex) && mem_match.size() > 1) {
                        GPUMemory memory = current_gpu.getMemory();
                        memory.total_memory_bytes = std::stoull(mem_match[1].str());
                        current_gpu.setMemory(memory);
                    }
                }
            } else if (line.find("Max Compute Units") != std::string::npos) {
                size_t pos = line.find(':');
                if (pos != std::string::npos) {
                    std::string cu_str = line.substr(pos + 1);
                    std::regex cu_regex("([0-9]+)");
                    std::smatch cu_match;
                    if (std::regex_search(cu_str, cu_match, cu_regex) && cu_match.size() > 1) {
                        GPUCompute compute = current_gpu.getCompute();
                        compute.compute_units = std::stoi(cu_match[1].str());
                        current_gpu.setCompute(compute);
                    }
                }
            }
        }
    }
    
    // Save the last GPU if we were in a device section
    if (in_device_section) {
        GPUAPIs apis = current_gpu.getAPIs();
        apis.supports_opencl = true;
        current_gpu.setAPIs(apis);
        m_gpus.push_back(current_gpu);
    }
#endif
}

void GPUDetector::detectMetalGPUs() {
#ifdef __APPLE__
    // On macOS, we can use system_profiler to get GPU information
    FILE* pipe = popen("system_profiler SPDisplaysDataType", "r");
    if (!pipe) {
        return;
    }
    
    char buffer[1024];
    std::string result = "";
    while (!feof(pipe)) {
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
    }
    pclose(pipe);
    
    // Parse the output (simplified)
    std::istringstream iss(result);
    std::string line;
    GPUInfo current_gpu;
    bool in_gpu_section = false;
    
    while (std::getline(iss, line)) {
        if (line.find("Chipset Model:") != std::string::npos) {
            // Start of a GPU section
            if (in_gpu_section) {
                // Save the previous GPU
                GPUAPIs apis = current_gpu.getAPIs();
                apis.supports_metal = true;
                current_gpu.setAPIs(apis);
                m_gpus.push_back(current_gpu);
            }
            
            in_gpu_section = true;
            current_gpu = GPUInfo();
            
            size_t pos = line.find(':');
            if (pos != std::string::npos) {
                current_gpu.setName(line.substr(pos + 1));
            }
            
            // Determine vendor and if it's integrated
            if (line.find("Intel") != std::string::npos) {
                current_gpu.setVendor(GPUVendor::INTEL);
                current_gpu.setIntegrated(true);
            } else if (line.find("AMD") != std::string::npos || line.find("Radeon") != std::string::npos) {
                current_gpu.setVendor(GPUVendor::AMD);
                current_gpu.setIntegrated(false);
            } else if (line.find("NVIDIA") != std::string::npos || line.find("GeForce") != std::string::npos) {
                current_gpu.setVendor(GPUVendor::NVIDIA);
                current_gpu.setIntegrated(false);
            } else if (line.find("Apple") != std::string::npos) {
                current_gpu.setVendor(GPUVendor::APPLE);
                current_gpu.setIntegrated(true);
            }
        } else if (in_gpu_section) {
            if (line.find("VRAM") != std::string::npos) {
                std::regex mem_regex("([0-9]+)\\s*([GM])B");
                std::smatch mem_match;
                if (std::regex_search(line, mem_match, mem_regex) && mem_match.size() > 2) {
                    GPUMemory memory = current_gpu.getMemory();
                    uint64_t mem_value = std::stoull(mem_match[1].str());
                    if (mem_match[2].str() == "G") {
                        memory.total_memory_bytes = mem_value * 1024 * 1024 * 1024;
                    } else {
                        memory.total_memory_bytes = mem_value * 1024 * 1024;
                    }
                    current_gpu.setMemory(memory);
                }
            } else if (line.find("Metal:") != std::string::npos && line.find("Supported") != std::string::npos) {
                GPUAPIs apis = current_gpu.getAPIs();
                apis.supports_metal = true;
                current_gpu.setAPIs(apis);
            }
        }
    }
    
    // Save the last GPU if we were in a GPU section
    if (in_gpu_section) {
        GPUAPIs apis = current_gpu.getAPIs();
        apis.supports_metal = true;
        current_gpu.setAPIs(apis);
        m_gpus.push_back(current_gpu);
    }
    
    // If we're on Apple Silicon, make sure we have the Apple GPU
    bool has_apple_gpu = false;
    for (const auto& gpu : m_gpus) {
        if (gpu.getVendor() == GPUVendor::APPLE) {
            has_apple_gpu = true;
            break;
        }
    }
    
    if (!has_apple_gpu) {
        // Check if we're on Apple Silicon
        char cpu_brand[256];
        size_t size = sizeof(cpu_brand);
        if (sysctlbyname("machdep.cpu.brand_string", &cpu_brand, &size, nullptr, 0) == 0) {
            std::string brand(cpu_brand);
            if (brand.find("Apple") != std::string::npos) {
                // We're on Apple Silicon, add the Apple GPU
                GPUInfo gpu;
                gpu.setVendor(GPUVendor::APPLE);
                gpu.setName("Apple M-series GPU");
                gpu.setIntegrated(true);
                
                GPUAPIs apis;
                apis.supports_metal = true;
                gpu.setAPIs(apis);
                
                // Estimate memory (shared with system)
                int64_t mem_size;
                size = sizeof(mem_size);
                if (sysctlbyname("hw.memsize", &mem_size, &size, nullptr, 0) == 0) {
                    GPUMemory memory;
                    memory.total_memory_bytes = static_cast<uint64_t>(mem_size) / 2;  // Assume half of system memory
                    gpu.setMemory(memory);
                }
                
                // Estimate compute units (varies by model)
                GPUCompute compute;
                compute.compute_units = 8;  // Conservative estimate
                gpu.setCompute(compute);
                
                m_gpus.push_back(gpu);
            }
        }
    }
#endif
}

void GPUDetector::detectDirectXGPUs() {
#ifdef _WIN32
    // On Windows, we could use DXGI to enumerate adapters
    // For simplicity, we'll use a basic approach with Windows Management Instrumentation (WMI)
    
    // This would normally use WMI to query GPU information
    // For simplicity, we'll just check if we can load the D3D11 DLL
    HMODULE d3d11Module = LoadLibraryA("d3d11.dll");
    if (d3d11Module != NULL) {
        // DirectX 11 is available, assume there's at least one GPU
        FreeLibrary(d3d11Module);
        
        // Try to get more information using dxdiag
        FILE* pipe = popen("dxdiag /t", "r");
        if (pipe) {
            char buffer[1024];
            std::string result = "";
            while (!feof(pipe)) {
                if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                    result += buffer;
                }
            }
            pclose(pipe);
            
            // Parse the output (simplified)
            std::istringstream iss(result);
            std::string line;
            GPUInfo current_gpu;
            bool in_display_section = false;
            
            while (std::getline(iss, line)) {
                if (line.find("Card name:") != std::string::npos) {
                    // Start of a display section
                    if (in_display_section) {
                        // Save the previous GPU
                        GPUAPIs apis = current_gpu.getAPIs();
                        apis.supports_directx = true;
                        current_gpu.setAPIs(apis);
                        m_gpus.push_back(current_gpu);
                    }
                    
                    in_display_section = true;
                    current_gpu = GPUInfo();
                    
                    size_t pos = line.find(':');
                    if (pos != std::string::npos) {
                        current_gpu.setName(line.substr(pos + 1));
                    }
                    
                    // Determine vendor
                    if (line.find("Intel") != std::string::npos) {
                        current_gpu.setVendor(GPUVendor::INTEL);
                        current_gpu.setIntegrated(true);
                    } else if (line.find("AMD") != std::string::npos || line.find("Radeon") != std::string::npos) {
                        current_gpu.setVendor(GPUVendor::AMD);
                        current_gpu.setIntegrated(false);
                    } else if (line.find("NVIDIA") != std::string::npos || line.find("GeForce") != std::string::npos) {
                        current_gpu.setVendor(GPUVendor::NVIDIA);
                        current_gpu.setIntegrated(false);
                    }
                } else if (in_display_section) {
                    if (line.find("Driver Version:") != std::string::npos) {
                        size_t pos = line.find(':');
                        if (pos != std::string::npos) {
                            current_gpu.setDriverVersion(line.substr(pos + 1));
                        }
                    } else if (line.find("Dedicated Memory:") != std::string::npos) {
                        std::regex mem_regex("([0-9]+)\\s*([GM])B");
                        std::smatch mem_match;
                        if (std::regex_search(line, mem_match, mem_regex) && mem_match.size() > 2) {
                            GPUMemory memory = current_gpu.getMemory();
                            uint64_t mem_value = std::stoull(mem_match[1].str());
                            if (mem_match[2].str() == "G") {
                                memory.total_memory_bytes = mem_value * 1024 * 1024 * 1024;
                            } else {
                                memory.total_memory_bytes = mem_value * 1024 * 1024;
                            }
                            current_gpu.setMemory(memory);
                        }
                    }
                }
            }
            
            // Save the last GPU if we were in a display section
            if (in_display_section) {
                GPUAPIs apis = current_gpu.getAPIs();
                apis.supports_directx = true;
                current_gpu.setAPIs(apis);
                m_gpus.push_back(current_gpu);
            }
        } else {
            // Fallback if dxdiag fails
            GPUInfo gpu;
            gpu.setName("DirectX Compatible GPU");
            
            GPUAPIs apis;
            apis.supports_directx = true;
            gpu.setAPIs(apis);
            
            m_gpus.push_back(gpu);
        }
    }
#endif
}

void GPUDetector::detectVulkanGPUs() {
    // This would normally use the Vulkan API to detect GPUs
    // For simplicity, we'll use a command-line approach here
    
#ifdef __linux__
    // Try using vulkaninfo on Linux
    FILE* pipe = popen("vulkaninfo --summary", "r");
    if (!pipe) {
        return;
    }
    
    char buffer[1024];
    std::string result = "";
    while (!feof(pipe)) {
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
    }
    pclose(pipe);
    
    // Parse the output (simplified)
    std::istringstream iss(result);
    std::string line;
    GPUInfo current_gpu;
    bool in_device_section = false;
    
    while (std::getline(iss, line)) {
        if (line.find("GPU") != std::string::npos && line.find("id") != std::string::npos) {
            // Start of a GPU device section
            if (in_device_section) {
                // Save the previous GPU
                GPUAPIs apis = current_gpu.getAPIs();
                apis.supports_vulkan = true;
                current_gpu.setAPIs(apis);
                m_gpus.push_back(current_gpu);
            }
            
            in_device_section = true;
            current_gpu = GPUInfo();
        } else if (in_device_section) {
            if (line.find("deviceName") != std::string::npos) {
                size_t pos = line.find('=');
                if (pos != std::string::npos) {
                    current_gpu.setName(line.substr(pos + 1));
                }
            } else if (line.find("driverVersion") != std::string::npos) {
                size_t pos = line.find('=');
                if (pos != std::string::npos) {
                    current_gpu.setDriverVersion(line.substr(pos + 1));
                }
            } else if (line.find("vendorID") != std::string::npos) {
                if (line.find("0x1002") != std::string::npos) {
                    current_gpu.setVendor(GPUVendor::AMD);
                } else if (line.find("0x10de") != std::string::npos) {
                    current_gpu.setVendor(GPUVendor::NVIDIA);
                } else if (line.find("0x8086") != std::string::npos) {
                    current_gpu.setVendor(GPUVendor::INTEL);
                }
            }
        }
    }
    
    // Save the last GPU if we were in a device section
    if (in_device_section) {
        GPUAPIs apis = current_gpu.getAPIs();
        apis.supports_vulkan = true;
        current_gpu.setAPIs(apis);
        m_gpus.push_back(current_gpu);
    }
#endif
}

void GPUDetector::detectUsingCommandLine() {
    // Fallback detection using command-line tools
    
#ifdef __linux__
    // Try using lspci on Linux
    FILE* pipe = popen("lspci | grep -i 'vga\\|3d\\|display'", "r");
    if (!pipe) {
        return;
    }
    
    char buffer[1024];
    std::string result = "";
    while (!feof(pipe)) {
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
    }
    pclose(pipe);
    
    // Parse the output
    std::istringstream iss(result);
    std::string line;
    while (std::getline(iss, line)) {
        GPUInfo gpu;
        gpu.setName(line);
        
        if (line.find("NVIDIA") != std::string::npos) {
            gpu.setVendor(GPUVendor::NVIDIA);
            gpu.setIntegrated(false);
        } else if (line.find("AMD") != std::string::npos || line.find("ATI") != std::string::npos || line.find("Radeon") != std::string::npos) {
            gpu.setVendor(GPUVendor::AMD);
            gpu.setIntegrated(false);
        } else if (line.find("Intel") != std::string::npos) {
            gpu.setVendor(GPUVendor::INTEL);
            gpu.setIntegrated(true);
        }
        
        m_gpus.push_back(gpu);
    }
#elif defined(__APPLE__)
    // Already handled in detectMetalGPUs()
#elif defined(_WIN32)
    // Try using wmic on Windows
    FILE* pipe = popen("wmic path win32_VideoController get Name, AdapterRAM, DriverVersion", "r");
    if (!pipe) {
        return;
    }
    
    char buffer[1024];
    std::string result = "";
    while (!feof(pipe)) {
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
    }
    pclose(pipe);
    
    // Parse the output
    std::istringstream iss(result);
    std::string line;
    bool header_skipped = false;
    
    while (std::getline(iss, line)) {
        if (!header_skipped) {
            header_skipped = true;
            continue;
        }
        
        if (line.empty() || line.find_first_not_of(" \t\r\n") == std::string::npos) {
            continue;
        }
        
        GPUInfo gpu;
        
        // Extract name (everything up to the first number, which is likely the adapter RAM)
        size_t name_end = line.find_first_of("0123456789");
        if (name_end != std::string::npos) {
            gpu.setName(line.substr(0, name_end));
        } else {
            gpu.setName(line);
        }
        
        // Determine vendor
        if (line.find("NVIDIA") != std::string::npos || line.find("GeForce") != std::string::npos) {
            gpu.setVendor(GPUVendor::NVIDIA);
            gpu.setIntegrated(false);
        } else if (line.find("AMD") != std::string::npos || line.find("Radeon") != std::string::npos) {
            gpu.setVendor(GPUVendor::AMD);
            gpu.setIntegrated(false);
        } else if (line.find("Intel") != std::string::npos) {
            gpu.setVendor(GPUVendor::INTEL);
            gpu.setIntegrated(true);
        }
        
        m_gpus.push_back(gpu);
    }
#endif
}

void GPUDetector::mergeGPUInfo() {
    // Merge duplicate GPU entries
    if (m_gpus.size() <= 1) {
        return;
    }
    
    std::vector<GPUInfo> merged_gpus;
    std::vector<bool> merged(m_gpus.size(), false);
    
    for (size_t i = 0; i < m_gpus.size(); i++) {
        if (merged[i]) {
            continue;
        }
        
        GPUInfo merged_gpu = m_gpus[i];
        merged[i] = true;
        
        for (size_t j = i + 1; j < m_gpus.size(); j++) {
            if (merged[j]) {
                continue;
            }
            
            // Check if GPUs are likely the same
            bool same_vendor = merged_gpu.getVendor() == m_gpus[j].getVendor();
            bool same_name = merged_gpu.getName().find(m_gpus[j].getName()) != std::string::npos ||
                             m_gpus[j].getName().find(merged_gpu.getName()) != std::string::npos;
            
            if (same_vendor && same_name) {
                // Merge GPU information
                merged[j] = true;
                
                // Merge APIs
                GPUAPIs apis = merged_gpu.getAPIs();
                GPUAPIs other_apis = m_gpus[j].getAPIs();
                apis.supports_cuda |= other_apis.supports_cuda;
                apis.supports_opencl |= other_apis.supports_opencl;
                apis.supports_metal |= other_apis.supports_metal;
                apis.supports_directx |= other_apis.supports_directx;
                apis.supports_vulkan |= other_apis.supports_vulkan;
                merged_gpu.setAPIs(apis);
                
                // Merge memory (take the larger value)
                GPUMemory memory = merged_gpu.getMemory();
                GPUMemory other_memory = m_gpus[j].getMemory();
                if (other_memory.total_memory_bytes > memory.total_memory_bytes) {
                    memory.total_memory_bytes = other_memory.total_memory_bytes;
                }
                if (other_memory.available_memory_bytes > memory.available_memory_bytes) {
                    memory.available_memory_bytes = other_memory.available_memory_bytes;
                }
                if (other_memory.memory_clock_mhz > memory.memory_clock_mhz) {
                    memory.memory_clock_mhz = other_memory.memory_clock_mhz;
                }
                if (other_memory.memory_bandwidth_gbps > memory.memory_bandwidth_gbps) {
                    memory.memory_bandwidth_gbps = other_memory.memory_bandwidth_gbps;
                }
                merged_gpu.setMemory(memory);
                
                // Merge compute (take the larger value)
                GPUCompute compute = merged_gpu.getCompute();
                GPUCompute other_compute = m_gpus[j].getCompute();
                if (other_compute.compute_units > compute.compute_units) {
                    compute.compute_units = other_compute.compute_units;
                }
                if (other_compute.cuda_cores > compute.cuda_cores) {
                    compute.cuda_cores = other_compute.cuda_cores;
                }
                if (other_compute.tensor_cores > compute.tensor_cores) {
                    compute.tensor_cores = other_compute.tensor_cores;
                }
                if (other_compute.rt_cores > compute.rt_cores) {
                    compute.rt_cores = other_compute.rt_cores;
                }
                if (other_compute.clock_mhz > compute.clock_mhz) {
                    compute.clock_mhz = other_compute.clock_mhz;
                }
                if (other_compute.tflops_fp32 > compute.tflops_fp32) {
                    compute.tflops_fp32 = other_compute.tflops_fp32;
                }
                if (other_compute.tflops_fp16 > compute.tflops_fp16) {
                    compute.tflops_fp16 = other_compute.tflops_fp16;
                }
                if (other_compute.cuda_compute_capability_major > compute.cuda_compute_capability_major ||
                    (other_compute.cuda_compute_capability_major == compute.cuda_compute_capability_major &&
                     other_compute.cuda_compute_capability_minor > compute.cuda_compute_capability_minor)) {
                    compute.cuda_compute_capability_major = other_compute.cuda_compute_capability_major;
                    compute.cuda_compute_capability_minor = other_compute.cuda_compute_capability_minor;
                }
                merged_gpu.setCompute(compute);
                
                // Prefer non-empty driver version
                if (merged_gpu.getDriverVersion().empty() && !m_gpus[j].getDriverVersion().empty()) {
                    merged_gpu.setDriverVersion(m_gpus[j].getDriverVersion());
                }
                
                // Prefer non-empty name
                if (merged_gpu.getName().empty() && !m_gpus[j].getName().empty()) {
                    merged_gpu.setName(m_gpus[j].getName());
                }
            }
        }
        
        merged_gpus.push_back(merged_gpu);
    }
    
    m_gpus = merged_gpus;
}

void GPUDetector::calculatePerformanceMetrics() {
    for (auto& gpu : m_gpus) {
        GPUCompute compute = gpu.getCompute();
        GPUMemory memory = gpu.getMemory();
        
        // Calculate CUDA cores based on compute units for AMD GPUs
        if (gpu.getVendor() == GPUVendor::AMD && compute.compute_units > 0 && compute.cuda_cores == 0) {
            // AMD compute units typically have 64 stream processors
            compute.cuda_cores = compute.compute_units * 64;
        }
        
        // Calculate theoretical FP32 performance
        if (compute.clock_mhz > 0) {
            if (compute.cuda_cores > 0) {
                // NVIDIA: TFLOPS = (cores * 2 * clock) / 1000
                compute.tflops_fp32 = (compute.cuda_cores * 2 * compute.clock_mhz) / 1000000.0;
            } else if (compute.compute_units > 0) {
                // AMD/Intel: Estimate based on compute units
                compute.tflops_fp32 = (compute.compute_units * 64 * 2 * compute.clock_mhz) / 1000000.0;
            }
        }
        
        // Estimate FP16 performance
        if (compute.tflops_fp32 > 0) {
            if (gpu.getVendor() == GPUVendor::NVIDIA && compute.cuda_compute_capability_major >= 7) {
                // NVIDIA Volta and newer: 2x FP32 for FP16
                compute.tflops_fp16 = compute.tflops_fp32 * 2;
            } else if (gpu.getVendor() == GPUVendor::AMD) {
                // AMD: 2x FP32 for FP16
                compute.tflops_fp16 = compute.tflops_fp32 * 2;
            } else if (gpu.getVendor() == GPUVendor::APPLE) {
                // Apple: 2x FP32 for FP16
                compute.tflops_fp16 = compute.tflops_fp32 * 2;
            } else {
                // Default: Same as FP32
                compute.tflops_fp16 = compute.tflops_fp32;
            }
        }
        
        // Estimate memory bandwidth
        if (memory.memory_clock_mhz > 0 && memory.total_memory_bytes > 0) {
            // Very rough estimate
            memory.memory_bandwidth_gbps = memory.memory_clock_mhz * 4 / 1000.0;
        }
        
        gpu.setCompute(compute);
        gpu.setMemory(memory);
    }
}

} // namespace mfp
