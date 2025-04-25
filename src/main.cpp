#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include "mfp_system.h"
#include "resource_manager.h"
#include "configuration_manager.h"

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [options] <command> [arguments]" << std::endl;
    std::cout << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  isprime <number>              Check if a number is prime" << std::endl;
    std::cout << "  nextprime <number>            Find the next prime number" << std::endl;
    std::cout << "  factorize <number>            Find prime factors of a number" << std::endl;
    std::cout << "  benchmark                     Run performance benchmark" << std::endl;
    std::cout << "  sysinfo                       Display system information" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --cpu-only                    Use CPU only" << std::endl;
    std::cout << "  --gpu-only                    Use GPU only" << std::endl;
    std::cout << "  --cuda-only                   Use CUDA GPU only" << std::endl;
    std::cout << "  --metal-only                  Use Metal GPU only" << std::endl;
    std::cout << "  --hybrid                      Use both CPU and GPU" << std::endl;
    std::cout << "  --method <1|2|3|auto>         Select MFP method" << std::endl;
    std::cout << "  --perf-log <on|off>           Enable/disable performance logging" << std::endl;
    std::cout << "  --config <file>               Load configuration from file" << std::endl;
    std::cout << "  --save-config <file>          Save configuration to file" << std::endl;
    std::cout << "  --profile <name>              Use specific configuration profile" << std::endl;
    std::cout << "  --help                        Display this help message" << std::endl;
    std::cout << "  --version                     Display version information" << std::endl;
}

void printVersion() {
    std::cout << "MFP Enhanced v" << mfp::config::getVersionString() << std::endl;
    std::cout << "CUDA Support: " << (mfp::config::hasCUDASupport() ? "Yes" : "No") << std::endl;
    std::cout << "Metal Support: " << (mfp::config::hasMetalSupport() ? "Yes" : "No") << std::endl;
    std::cout << "Modular Factorization Pattern algorithm by Marlon F. Polegato" << std::endl;
    std::cout << "https://www.linkedin.com/in/marlonpolegato/" << std::endl;
}

int main(int argc, char* argv[]) {
    // Check for help or version flags
    if (argc < 2 || std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h") {
        printUsage(argv[0]);
        return 0;
    }
    
    if (std::string(argv[1]) == "--version" || std::string(argv[1]) == "-v") {
        printVersion();
        return 0;
    }
    
    // Create resource manager
    mfp::resource::ResourceManager resource_manager;
    
    // Initialize resource manager
    if (!resource_manager.initialize()) {
        std::cerr << "Failed to initialize resource manager" << std::endl;
        return 1;
    }
    
    // Create configuration manager
    mfp::config::ConfigurationManager config_manager;
    
    // Initialize configuration manager
    if (!config_manager.initialize(&resource_manager)) {
        std::cerr << "Failed to initialize configuration manager" << std::endl;
        return 1;
    }
    
    // Auto-configure based on hardware
    if (!config_manager.autoConfigureForHardware()) {
        std::cerr << "Failed to auto-configure for hardware" << std::endl;
        return 1;
    }
    
    // Parse command line arguments
    std::string command;
    std::string number;
    std::string configFile;
    std::string saveConfigFile;
    std::string profileName;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--cpu-only") {
            resource_manager.setAllocationMode(mfp::resource::AllocationMode::CPU_ONLY);
        } else if (arg == "--gpu-only") {
            resource_manager.setAllocationMode(mfp::resource::AllocationMode::GPU_ONLY);
        } else if (arg == "--cuda-only") {
            resource_manager.setAllocationMode(mfp::resource::AllocationMode::CUDA_ONLY);
        } else if (arg == "--metal-only") {
            resource_manager.setAllocationMode(mfp::resource::AllocationMode::METAL_ONLY);
        } else if (arg == "--hybrid") {
            resource_manager.setAllocationMode(mfp::resource::AllocationMode::HYBRID);
        } else if (arg == "--method") {
            if (i + 1 < argc) {
                std::string method = argv[++i];
                if (method == "1") {
                    resource_manager.setMFPMethod(mfp::resource::MFPMethod::METHOD_1);
                } else if (method == "2") {
                    resource_manager.setMFPMethod(mfp::resource::MFPMethod::METHOD_2);
                } else if (method == "3") {
                    resource_manager.setMFPMethod(mfp::resource::MFPMethod::METHOD_3);
                } else if (method == "auto") {
                    resource_manager.setMFPMethod(mfp::resource::MFPMethod::AUTO);
                } else {
                    std::cerr << "Invalid method: " << method << std::endl;
                    return 1;
                }
            } else {
                std::cerr << "Missing method argument" << std::endl;
                return 1;
            }
        } else if (arg == "--perf-log") {
            if (i + 1 < argc) {
                std::string value = argv[++i];
                if (value == "on") {
                    resource_manager.setPerformanceLogging(true);
                } else if (value == "off") {
                    resource_manager.setPerformanceLogging(false);
                } else {
                    std::cerr << "Invalid performance logging value: " << value << std::endl;
                    return 1;
                }
            } else {
                std::cerr << "Missing performance logging argument" << std::endl;
                return 1;
            }
        } else if (arg == "--config") {
            if (i + 1 < argc) {
                configFile = argv[++i];
            } else {
                std::cerr << "Missing config file argument" << std::endl;
                return 1;
            }
        } else if (arg == "--save-config") {
            if (i + 1 < argc) {
                saveConfigFile = argv[++i];
            } else {
                std::cerr << "Missing save config file argument" << std::endl;
                return 1;
            }
        } else if (arg == "--profile") {
            if (i + 1 < argc) {
                profileName = argv[++i];
            } else {
                std::cerr << "Missing profile name argument" << std::endl;
                return 1;
            }
        } else if (arg[0] != '-') {
            // Non-option argument
            if (command.empty()) {
                command = arg;
            } else if (number.empty() && (command == "isprime" || command == "nextprime" || command == "factorize")) {
                number = arg;
            }
        }
    }
    
    // Load configuration if specified
    if (!configFile.empty()) {
        if (!config_manager.loadConfiguration(configFile)) {
            std::cerr << "Failed to load configuration from " << configFile << std::endl;
            return 1;
        }
    }
    
    // Set profile if specified
    if (!profileName.empty()) {
        config_manager.setCurrentProfile(profileName);
    }
    
    // Apply configuration
    config_manager.applyConfiguration();
    
    // Save configuration if specified
    if (!saveConfigFile.empty()) {
        if (!config_manager.saveConfiguration(saveConfigFile)) {
            std::cerr << "Failed to save configuration to " << saveConfigFile << std::endl;
            return 1;
        }
    }
    
    // Create MFP system
    mfp::MFPSystem system(&resource_manager);
    
    // Execute command
    if (command == "isprime") {
        if (number.empty()) {
            std::cerr << "Missing number argument" << std::endl;
            return 1;
        }
        
        auto start_time = std::chrono::high_resolution_clock::now();
        bool is_prime = system.isPrime(number);
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        
        std::cout << number << " is " << (is_prime ? "prime" : "not prime") << std::endl;
        std::cout << "Execution time: " << duration << " ms" << std::endl;
        
        if (resource_manager.getPerformanceLogging()) {
            std::cout << "\nPerformance Metrics:" << std::endl;
            std::cout << resource_manager.getPerformanceMetrics() << std::endl;
        }
    } else if (command == "nextprime") {
        if (number.empty()) {
            std::cerr << "Missing number argument" << std::endl;
            return 1;
        }
        
        auto start_time = std::chrono::high_resolution_clock::now();
        std::string next_prime = system.findNextPrime(number);
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        
        std::cout << "Next prime after " << number << " is " << next_prime << std::endl;
        std::cout << "Execution time: " << duration << " ms" << std::endl;
        
        if (resource_manager.getPerformanceLogging()) {
            std::cout << "\nPerformance Metrics:" << std::endl;
            std::cout << resource_manager.getPerformanceMetrics() << std::endl;
        }
    } else if (command == "factorize") {
        if (number.empty()) {
            std::cerr << "Missing number argument" << std::endl;
            return 1;
        }
        
        std::vector<std::string> factors;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        bool success = system.findPrimeFactors(number, factors);
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        
        if (success) {
            std::cout << "Prime factors of " << number << ":" << std::endl;
            for (const auto& factor : factors) {
                std::cout << factor << std::endl;
            }
        } else {
            std::cout << "Failed to factorize " << number << std::endl;
        }
        
        std::cout << "Execution time: " << duration << " ms" << std::endl;
        
        if (resource_manager.getPerformanceLogging()) {
            std::cout << "\nPerformance Metrics:" << std::endl;
            std::cout << resource_manager.getPerformanceMetrics() << std::endl;
        }
    } else if (command == "benchmark") {
        std::cout << "Running benchmark..." << std::endl;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        mfp::resource::BenchmarkResult result = resource_manager.runBenchmark();
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        
        std::cout << "Benchmark completed in " << duration << " ms" << std::endl;
        std::cout << "\nBenchmark Results:" << std::endl;
        std::cout << "CPU Score: " << result.cpu_score << std::endl;
        std::cout << "CUDA Score: " << result.cuda_score << std::endl;
        std::cout << "Metal Score: " << result.metal_score << std::endl;
        std::cout << "Best Device: " << result.best_device << std::endl;
        
        if (!result.details.empty()) {
            std::cout << "\nDetails:" << std::endl;
            std::cout << result.details << std::endl;
        }
    } else if (command == "sysinfo") {
        std::cout << "System Information:" << std::endl;
        std::cout << resource_manager.getSystemInfo() << std::endl;
        
        std::cout << "\nConfiguration:" << std::endl;
        std::cout << config_manager.getConfigurationSummary() << std::endl;
    } else {
        std::cerr << "Unknown command: " << command << std::endl;
        printUsage(argv[0]);
        return 1;
    }
    
    return 0;
}
