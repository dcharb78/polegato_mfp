#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <fstream>
#include <iomanip>
#include <memory>
#include <random>
#include <algorithm>

#include "cpu_detector.h"
#include "memory_storage_detector.h"
#include "gpu_detector.h"
#include "resource_manager.h"
#include "configuration_manager.h"

// Test case base class
class TestCase {
public:
    virtual ~TestCase() = default;
    virtual std::string getName() const = 0;
    virtual std::string getDescription() const = 0;
    virtual bool run() = 0;
    virtual std::string getResults() const = 0;
};

// Hardware detection test
class HardwareDetectionTest : public TestCase {
public:
    HardwareDetectionTest() {}
    
    std::string getName() const override {
        return "Hardware Detection Test";
    }
    
    std::string getDescription() const override {
        return "Tests the detection of CPU, memory, storage, and GPU capabilities";
    }
    
    bool run() override {
        std::cout << "Running " << getName() << "..." << std::endl;
        
        // Test CPU detection
        mfp::system::CPUDetector cpu_detector;
        mfp::system::CPUInfo cpu_info;
        
        bool cpu_success = cpu_detector.detect(cpu_info);
        results_ += "CPU Detection: " + std::string(cpu_success ? "SUCCESS" : "FAILURE") + "\n";
        
        if (cpu_success) {
            results_ += "  Model: " + cpu_info.model_name + "\n";
            results_ += "  Architecture: " + cpu_info.architecture + "\n";
            results_ += "  Physical Cores: " + std::to_string(cpu_info.physical_cores) + "\n";
            results_ += "  Logical Cores: " + std::to_string(cpu_info.logical_cores) + "\n";
            results_ += "  Base Frequency: " + std::to_string(cpu_info.base_frequency_mhz) + " MHz\n";
            
            results_ += "  Features: ";
            for (size_t i = 0; i < cpu_info.features.size(); ++i) {
                if (i > 0) {
                    results_ += ", ";
                }
                results_ += cpu_info.features[i];
            }
            results_ += "\n";
        }
        
        // Test memory detection
        mfp::system::MemoryStorageDetector memory_detector;
        mfp::system::MemoryInfo memory_info;
        
        bool memory_success = memory_detector.detectMemory(memory_info);
        results_ += "\nMemory Detection: " + std::string(memory_success ? "SUCCESS" : "FAILURE") + "\n";
        
        if (memory_success) {
            results_ += "  Total Memory: " + std::to_string(memory_info.total_memory_bytes / (1024 * 1024 * 1024)) + " GB\n";
            results_ += "  Available Memory: " + std::to_string(memory_info.available_memory_bytes / (1024 * 1024 * 1024)) + " GB\n";
            
            if (memory_info.memory_type != mfp::system::MemoryType::UNKNOWN) {
                results_ += "  Memory Type: " + std::to_string(static_cast<int>(memory_info.memory_type)) + "\n";
            }
            
            if (memory_info.memory_speed_mhz > 0) {
                results_ += "  Memory Speed: " + std::to_string(memory_info.memory_speed_mhz) + " MHz\n";
            }
        }
        
        // Test storage detection
        mfp::system::StorageInfo storage_info;
        
        bool storage_success = memory_detector.detectStorage(storage_info);
        results_ += "\nStorage Detection: " + std::string(storage_success ? "SUCCESS" : "FAILURE") + "\n";
        
        if (storage_success) {
            results_ += "  Total Storage: " + std::to_string(storage_info.total_bytes / (1024 * 1024 * 1024)) + " GB\n";
            results_ += "  Available Storage: " + std::to_string(storage_info.available_bytes / (1024 * 1024 * 1024)) + " GB\n";
            
            results_ += "  Storage Devices:\n";
            for (const auto& device : storage_info.devices) {
                results_ += "    " + device.name + " (" + std::to_string(device.size_bytes / (1024 * 1024 * 1024)) + " GB)\n";
                results_ += "      Type: " + std::to_string(static_cast<int>(device.type)) + "\n";
                results_ += "      Read Speed: " + std::to_string(device.read_speed_mbps) + " MB/s\n";
                results_ += "      Write Speed: " + std::to_string(device.write_speed_mbps) + " MB/s\n";
            }
        }
        
        // Test GPU detection
        mfp::system::GPUDetector gpu_detector;
        std::vector<mfp::system::GPUInfo> gpu_info;
        
        bool gpu_success = gpu_detector.detect(gpu_info);
        results_ += "\nGPU Detection: " + std::string(gpu_success ? "SUCCESS" : "FAILURE") + "\n";
        
        if (gpu_success) {
            if (gpu_info.empty()) {
                results_ += "  No GPUs detected\n";
            } else {
                for (size_t i = 0; i < gpu_info.size(); ++i) {
                    const auto& gpu = gpu_info[i];
                    
                    results_ += "  GPU " + std::to_string(i) + ": " + gpu.name + "\n";
                    results_ += "    Vendor: " + std::to_string(static_cast<int>(gpu.vendor)) + "\n";
                    results_ += "    Architecture: " + std::to_string(static_cast<int>(gpu.architecture)) + "\n";
                    
                    results_ += "    API Support: ";
                    for (size_t j = 0; j < gpu.api_support.size(); ++j) {
                        if (j > 0) {
                            results_ += ", ";
                        }
                        results_ += std::to_string(static_cast<int>(gpu.api_support[j]));
                    }
                    results_ += "\n";
                    
                    results_ += "    Memory: " + std::to_string(gpu.memory_info.total_memory_bytes / (1024 * 1024 * 1024)) + " GB\n";
                    
                    if (gpu.is_integrated) {
                        results_ += "    Type: Integrated\n";
                    } else {
                        results_ += "    Type: Discrete\n";
                    }
                }
            }
        }
        
        return cpu_success && memory_success && storage_success && gpu_success;
    }
    
    std::string getResults() const override {
        return results_;
    }
    
private:
    std::string results_;
};

// Resource allocation test
class ResourceAllocationTest : public TestCase {
public:
    ResourceAllocationTest() {}
    
    std::string getName() const override {
        return "Resource Allocation Test";
    }
    
    std::string getDescription() const override {
        return "Tests the allocation of resources for different modes";
    }
    
    bool run() override {
        std::cout << "Running " << getName() << "..." << std::endl;
        
        // Create resource manager
        mfp::resource::ResourceManager resource_manager;
        
        // Initialize resource manager
        bool init_success = resource_manager.initialize();
        results_ += "Resource Manager Initialization: " + std::string(init_success ? "SUCCESS" : "FAILURE") + "\n";
        
        if (!init_success) {
            return false;
        }
        
        // Test AUTO allocation mode
        resource_manager.setAllocationMode(mfp::resource::AllocationMode::AUTO);
        mfp::resource::AllocationResult auto_result = resource_manager.allocateResources();
        
        results_ += "\nAUTO Allocation Mode:\n";
        results_ += "  Success: " + std::string(auto_result.success ? "YES" : "NO") + "\n";
        if (auto_result.success) {
            results_ += "  Device Type: " + auto_result.device_type + "\n";
        } else {
            results_ += "  Error: " + auto_result.error_message + "\n";
        }
        
        // Test CPU_ONLY allocation mode
        resource_manager.setAllocationMode(mfp::resource::AllocationMode::CPU_ONLY);
        mfp::resource::AllocationResult cpu_result = resource_manager.allocateResources();
        
        results_ += "\nCPU_ONLY Allocation Mode:\n";
        results_ += "  Success: " + std::string(cpu_result.success ? "YES" : "NO") + "\n";
        if (cpu_result.success) {
            results_ += "  Device Type: " + cpu_result.device_type + "\n";
        } else {
            results_ += "  Error: " + cpu_result.error_message + "\n";
        }
        
        // Test GPU_ONLY allocation mode
        resource_manager.setAllocationMode(mfp::resource::AllocationMode::GPU_ONLY);
        mfp::resource::AllocationResult gpu_result = resource_manager.allocateResources();
        
        results_ += "\nGPU_ONLY Allocation Mode:\n";
        results_ += "  Success: " + std::string(gpu_result.success ? "YES" : "NO") + "\n";
        if (gpu_result.success) {
            results_ += "  Device Type: " + gpu_result.device_type + "\n";
        } else {
            results_ += "  Error: " + gpu_result.error_message + "\n";
        }
        
        // Test HYBRID allocation mode
        resource_manager.setAllocationMode(mfp::resource::AllocationMode::HYBRID);
        mfp::resource::AllocationResult hybrid_result = resource_manager.allocateResources();
        
        results_ += "\nHYBRID Allocation Mode:\n";
        results_ += "  Success: " + std::string(hybrid_result.success ? "YES" : "NO") + "\n";
        if (hybrid_result.success) {
            results_ += "  Device Type: " + hybrid_result.device_type + "\n";
        } else {
            results_ += "  Error: " + hybrid_result.error_message + "\n";
        }
        
        // Run benchmark
        mfp::resource::BenchmarkResult benchmark = resource_manager.runBenchmark();
        
        results_ += "\nBenchmark Results:\n";
        results_ += "  CPU Score: " + std::to_string(benchmark.cpu_score) + "\n";
        results_ += "  CUDA Score: " + std::to_string(benchmark.cuda_score) + "\n";
        results_ += "  Metal Score: " + std::to_string(benchmark.metal_score) + "\n";
        results_ += "  Best Device: " + benchmark.best_device + "\n";
        
        // Get system information
        results_ += "\nSystem Information:\n";
        results_ += resource_manager.getSystemInfo();
        
        return true;
    }
    
    std::string getResults() const override {
        return results_;
    }
    
private:
    std::string results_;
};

// Auto configuration test
class AutoConfigurationTest : public TestCase {
public:
    AutoConfigurationTest() {}
    
    std::string getName() const override {
        return "Auto Configuration Test";
    }
    
    std::string getDescription() const override {
        return "Tests the automatic configuration based on hardware capabilities";
    }
    
    bool run() override {
        std::cout << "Running " << getName() << "..." << std::endl;
        
        // Create resource manager
        mfp::resource::ResourceManager resource_manager;
        
        // Initialize resource manager
        bool rm_init_success = resource_manager.initialize();
        results_ += "Resource Manager Initialization: " + std::string(rm_init_success ? "SUCCESS" : "FAILURE") + "\n";
        
        if (!rm_init_success) {
            return false;
        }
        
        // Create configuration manager
        mfp::config::ConfigurationManager config_manager;
        
        // Initialize configuration manager
        bool cm_init_success = config_manager.initialize(&resource_manager);
        results_ += "Configuration Manager Initialization: " + std::string(cm_init_success ? "SUCCESS" : "FAILURE") + "\n";
        
        if (!cm_init_success) {
            return false;
        }
        
        // Auto-configure based on hardware
        bool auto_config_success = config_manager.autoConfigureForHardware();
        results_ += "Auto Configuration: " + std::string(auto_config_success ? "SUCCESS" : "FAILURE") + "\n";
        
        if (!auto_config_success) {
            return false;
        }
        
        // Get configuration summary
        results_ += "\nConfiguration Summary:\n";
        results_ += config_manager.getConfigurationSummary();
        
        // Test different hardware profiles
        results_ += "\nTesting Different Hardware Profiles:\n";
        
        // Low-end hardware profile
        config_manager.setCurrentProfile("low_end");
        results_ += "\nLow-End Hardware Profile:\n";
        results_ += config_manager.getConfigurationSummary();
        
        // Mid-range hardware profile
        config_manager.setCurrentProfile("mid_range");
        results_ += "\nMid-Range Hardware Profile:\n";
        results_ += config_manager.getConfigurationSummary();
        
        // High-end hardware profile
        config_manager.setCurrentProfile("high_end");
        results_ += "\nHigh-End Hardware Profile:\n";
        results_ += config_manager.getConfigurationSummary();
        
        // Server hardware profile
        config_manager.setCurrentProfile("server");
        results_ += "\nServer Hardware Profile:\n";
        results_ += config_manager.getConfigurationSummary();
        
        // Workstation hardware profile
        config_manager.setCurrentProfile("workstation");
        results_ += "\nWorkstation Hardware Profile:\n";
        results_ += config_manager.getConfigurationSummary();
        
        // Create custom profile
        auto custom_profile = config_manager.createProfile("custom_test", mfp::config::HardwareClass::CUSTOM);
        custom_profile->setParameter("allocation_mode", "cpu_only", false);
        custom_profile->setParameter("mfp_method", "method2", false);
        custom_profile->setParameter("thread_count", "4", false);
        custom_profile->setParameter("memory_limit_mb", "2048", false);
        
        config_manager.setCurrentProfile("custom_test");
        results_ += "\nCustom Hardware Profile:\n";
        results_ += config_manager.getConfigurationSummary();
        
        // Save configuration to file
        bool save_success = config_manager.saveConfiguration("test_config.cfg");
        results_ += "\nSave Configuration: " + std::string(save_success ? "SUCCESS" : "FAILURE") + "\n";
        
        // Load configuration from file
        bool load_success = config_manager.loadConfiguration("test_config.cfg");
        results_ += "Load Configuration: " + std::string(load_success ? "SUCCESS" : "FAILURE") + "\n";
        
        return true;
    }
    
    std::string getResults() const override {
        return results_;
    }
    
private:
    std::string results_;
};

// MFP performance test
class MFPPerformanceTest : public TestCase {
public:
    MFPPerformanceTest() {}
    
    std::string getName() const override {
        return "MFP Performance Test";
    }
    
    std::string getDescription() const override {
        return "Tests the performance of MFP methods on different hardware configurations";
    }
    
    bool run() override {
        std::cout << "Running " << getName() << "..." << std::endl;
        
        // Create resource manager
        mfp::resource::ResourceManager resource_manager;
        
        // Initialize resource manager
        bool rm_init_success = resource_manager.initialize();
        results_ += "Resource Manager Initialization: " + std::string(rm_init_success ? "SUCCESS" : "FAILURE") + "\n";
        
        if (!rm_init_success) {
            return false;
        }
        
        // Create configuration manager
        mfp::config::ConfigurationManager config_manager;
        
        // Initialize configuration manager
        bool cm_init_success = config_manager.initialize(&resource_manager);
        results_ += "Configuration Manager Initialization: " + std::string(cm_init_success ? "SUCCESS" : "FAILURE") + "\n";
        
        if (!cm_init_success) {
            return false;
        }
        
        // Auto-configure based on hardware
        bool auto_config_success = config_manager.autoConfigureForHardware();
        results_ += "Auto Configuration: " + std::string(auto_config_success ? "SUCCESS" : "FAILURE") + "\n\n";
        
        // Test different MFP methods with different number sizes
        results_ += "Testing MFP Methods with Different Number Sizes:\n";
        
        // Generate test numbers
        std::vector<std::string> test_numbers = {
            "12345",                                    // Small number (5 digits)
            "1234567890123456789",                      // Medium number (19 digits)
            "12345678901234567890123456789012345678901234567890"  // Large number (50 digits)
        };
        
        // Test each MFP method
        std::vector<mfp::resource::MFPMethod> methods = {
            mfp::resource::MFPMethod::METHOD_1,
            mfp::resource::MFPMethod::METHOD_2,
            mfp::resource::MFPMethod::METHOD_3,
            mfp::resource::MFPMethod::AUTO
        };
        
        std::vector<std::string> method_names = {
            "METHOD_1 (Expanded q Factorization)",
            "METHOD_2 (Ultrafast with Structural Filter)",
            "METHOD_3 (Parallelized with Dynamic Blocks)",
            "AUTO (Automatic Selection)"
        };
        
        // Test each allocation mode
        std::vector<mfp::resource::AllocationMode> modes = {
            mfp::resource::AllocationMode::CPU_ONLY,
            mfp::resource::AllocationMode::GPU_ONLY,
            mfp::resource::AllocationMode::HYBRID,
            mfp::resource::AllocationMode::AUTO
        };
        
        std::vector<std::string> mode_names = {
            "CPU_ONLY",
            "GPU_ONLY",
            "HYBRID",
            "AUTO"
        };
        
        // Enable performance logging
        resource_manager.setPerformanceLogging(true);
        
        // Run tests
        for (size_t mode_idx = 0; mode_idx < modes.size(); ++mode_idx) {
            auto mode = modes[mode_idx];
            auto mode_name = mode_names[mode_idx];
            
            results_ += "\nAllocation Mode: " + mode_name + "\n";
            resource_manager.setAllocationMode(mode);
            
            for (size_t method_idx = 0; method_idx < methods.size(); ++method_idx) {
                auto method = methods[method_idx];
                auto method_name = method_names[method_idx];
                
                results_ += "  MFP Method: " + method_name + "\n";
                resource_manager.setMFPMethod(method);
                
                for (size_t num_idx = 0; num_idx < test_numbers.size(); ++num_idx) {
                    auto number = test_numbers[num_idx];
                    
                    results_ += "    Number Size: " + std::to_string(number.size()) + " digits\n";
                    
                    // Allocate resources
                    mfp::resource::AllocationResult alloc_result = resource_manager.allocateResources();
                    
                    if (!alloc_result.success) {
                        results_ += "      Resource Allocation Failed: " + alloc_result.error_message + "\n";
                        continue;
                    }
                    
                    // Run MFP
                    auto start_time = std::chrono::high_resolution_clock::now();
                    
                    std::vector<std::string> factors;
                    bool mfp_success = resource_manager.runMFP(number, factors);
                    
                    auto end_time = std::chrono::high_resolution_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
                    
                    results_ += "      Execution Time: " + std::to_string(duration) + " ms\n";
                    results_ += "      Success: " + std::string(mfp_success ? "YES" : "NO") + "\n";
                    
                    if (mfp_success && !factors.empty()) {
                        results_ += "      Factors Found: " + std::to_string(factors.size()) + "\n";
                    }
                }
            }
        }
        
        // Get performance metrics
        results_ += "\nPerformance Metrics:\n";
        results_ += resource_manager.getPerformanceMetrics();
        
        return true;
    }
    
    std::string getResults() const override {
        return results_;
    }
    
private:
    std::string results_;
};

// Main test runner
int main() {
    std::cout << "MFP System Test Suite" << std::endl;
    std::cout << "====================" << std::endl;
    
    // Create test cases
    std::vector<std::unique_ptr<TestCase>> tests;
    tests.push_back(std::make_unique<HardwareDetectionTest>());
    tests.push_back(std::make_unique<ResourceAllocationTest>());
    tests.push_back(std::make_unique<AutoConfigurationTest>());
    tests.push_back(std::make_unique<MFPPerformanceTest>());
    
    // Run tests
    std::vector<bool> results;
    std::vector<std::string> result_details;
    
    for (const auto& test : tests) {
        std::cout << "\n" << test->getName() << std::endl;
        std::cout << test->getDescription() << std::endl;
        std::cout << std::string(test->getDescription().size(), '-') << std::endl;
        
        bool result = test->run();
        results.push_back(result);
        result_details.push_back(test->getResults());
        
        std::cout << "Result: " << (result ? "PASS" : "FAIL") << std::endl;
    }
    
    // Print summary
    std::cout << "\nTest Summary" << std::endl;
    std::cout << "===========" << std::endl;
    
    int pass_count = 0;
    for (size_t i = 0; i < tests.size(); ++i) {
        std::cout << tests[i]->getName() << ": " << (results[i] ? "PASS" : "FAIL") << std::endl;
        if (results[i]) {
            pass_count++;
        }
    }
    
    std::cout << "\nPassed " << pass_count << " of " << tests.size() << " tests" << std::endl;
    
    // Save detailed results to file
    std::ofstream result_file("test_results.txt");
    if (result_file.is_open()) {
        result_file << "MFP System Test Results" << std::endl;
        result_file << "======================" << std::endl;
        
        for (size_t i = 0; i < tests.size(); ++i) {
            result_file << "\n" << tests[i]->getName() << std::endl;
            result_file << tests[i]->getDescription() << std::endl;
            result_file << std::string(tests[i]->getDescription().size(), '-') << std::endl;
            result_file << "Result: " << (results[i] ? "PASS" : "FAIL") << std::endl;
            result_file << "\nDetails:\n" << result_details[i] << std::endl;
        }
        
        result_file.close();
        std::cout << "\nDetailed results saved to test_results.txt" << std::endl;
    }
    
    return pass_count == tests.size() ? 0 : 1;
}
