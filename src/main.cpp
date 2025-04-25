#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include "mfp_system.h"

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [options] <command> [arguments]" << std::endl;
    std::cout << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  isprime <number>              Check if a number is prime" << std::endl;
    std::cout << "  factorize <number>            Find prime factors of a number" << std::endl;
    std::cout << "  nextprime <number>            Find the next prime number" << std::endl;
    std::cout << "  benchmark <number>            Run benchmark on all methods" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --method <1|2|3|auto>         Select MFP method (default: auto)" << std::endl;
    std::cout << "  --threads <num>               Number of threads to use (default: all cores)" << std::endl;
    std::cout << "  --help                        Display this help message" << std::endl;
    std::cout << "  --version                     Display version information" << std::endl;
}

void printVersion() {
    std::cout << "MFP Implementation v1.0.0" << std::endl;
    std::cout << "Modular Factorization Pattern algorithm by Marlon F. Polegato" << std::endl;
    std::cout << "https://www.linkedin.com/in/marlonpolegato/" << std::endl;
}

int main(int argc, char* argv[]) {
    // Default values
    mfp::MFPMethodType method = mfp::MFPMethodType::AUTO;
    int numThreads = 0; // 0 means use all available cores
    std::string command;
    std::string number;
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "--version" || arg == "-v") {
            printVersion();
            return 0;
        } else if (arg == "--method" || arg == "-m") {
            if (i + 1 < argc) {
                std::string methodStr = argv[++i];
                if (methodStr == "1") {
                    method = mfp::MFPMethodType::METHOD_1;
                } else if (methodStr == "2") {
                    method = mfp::MFPMethodType::METHOD_2;
                } else if (methodStr == "3") {
                    method = mfp::MFPMethodType::METHOD_3;
                } else if (methodStr == "auto") {
                    method = mfp::MFPMethodType::AUTO;
                } else {
                    std::cerr << "Invalid method: " << methodStr << std::endl;
                    return 1;
                }
            } else {
                std::cerr << "Missing method argument" << std::endl;
                return 1;
            }
        } else if (arg == "--threads" || arg == "-t") {
            if (i + 1 < argc) {
                try {
                    numThreads = std::stoi(argv[++i]);
                    if (numThreads < 0) {
                        std::cerr << "Number of threads must be non-negative" << std::endl;
                        return 1;
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Invalid number of threads: " << argv[i] << std::endl;
                    return 1;
                }
            } else {
                std::cerr << "Missing threads argument" << std::endl;
                return 1;
            }
        } else if (command.empty()) {
            command = arg;
        } else if (number.empty()) {
            number = arg;
        }
    }
    
    // Check if command is provided
    if (command.empty()) {
        std::cerr << "No command specified" << std::endl;
        printUsage(argv[0]);
        return 1;
    }
    
    // Create MFP system
    mfp::MFPSystem mfpSystem(method, numThreads);
    
    // Execute command
    if (command == "isprime") {
        if (number.empty()) {
            std::cerr << "Missing number argument" << std::endl;
            return 1;
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        bool isPrime = mfpSystem.isPrime(number);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        std::cout << number << " is " << (isPrime ? "prime" : "not prime") << std::endl;
        std::cout << "Time: " << duration << " ms" << std::endl;
    } else if (command == "factorize") {
        if (number.empty()) {
            std::cerr << "Missing number argument" << std::endl;
            return 1;
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        std::vector<std::string> factors = mfpSystem.factorize(number);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        std::cout << "Factors of " << number << ":" << std::endl;
        for (const auto& factor : factors) {
            std::cout << factor << std::endl;
        }
        std::cout << "Time: " << duration << " ms" << std::endl;
    } else if (command == "nextprime") {
        if (number.empty()) {
            std::cerr << "Missing number argument" << std::endl;
            return 1;
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        std::string nextPrime = mfpSystem.findNextPrime(number);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        std::cout << "Next prime after " << number << " is " << nextPrime << std::endl;
        std::cout << "Time: " << duration << " ms" << std::endl;
    } else if (command == "benchmark") {
        if (number.empty()) {
            std::cerr << "Missing number argument" << std::endl;
            return 1;
        }
        
        std::cout << "Benchmarking MFP methods for number: " << number << std::endl;
        
        // Method 1
        mfpSystem.setMethod(mfp::MFPMethodType::METHOD_1);
        auto start1 = std::chrono::high_resolution_clock::now();
        bool isPrime1 = mfpSystem.isPrime(number);
        auto end1 = std::chrono::high_resolution_clock::now();
        auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1).count();
        
        // Method 2
        mfpSystem.setMethod(mfp::MFPMethodType::METHOD_2);
        auto start2 = std::chrono::high_resolution_clock::now();
        bool isPrime2 = mfpSystem.isPrime(number);
        auto end2 = std::chrono::high_resolution_clock::now();
        auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2).count();
        
        // Method 3
        mfpSystem.setMethod(mfp::MFPMethodType::METHOD_3);
        auto start3 = std::chrono::high_resolution_clock::now();
        bool isPrime3 = mfpSystem.isPrime(number);
        auto end3 = std::chrono::high_resolution_clock::now();
        auto duration3 = std::chrono::duration_cast<std::chrono::milliseconds>(end3 - start3).count();
        
        std::cout << "Results:" << std::endl;
        std::cout << "Method 1 (Expanded q Factorization): " << duration1 << " ms, " << (isPrime1 ? "prime" : "not prime") << std::endl;
        std::cout << "Method 2 (Ultrafast with Structural Filter): " << duration2 << " ms, " << (isPrime2 ? "prime" : "not prime") << std::endl;
        std::cout << "Method 3 (Parallelized with Dynamic Blocks): " << duration3 << " ms, " << (isPrime3 ? "prime" : "not prime") << std::endl;
        
        // Determine fastest method
        if (duration1 <= duration2 && duration1 <= duration3) {
            std::cout << "Method 1 is fastest" << std::endl;
        } else if (duration2 <= duration1 && duration2 <= duration3) {
            std::cout << "Method 2 is fastest" << std::endl;
        } else {
            std::cout << "Method 3 is fastest" << std::endl;
        }
    } else {
        std::cerr << "Unknown command: " << command << std::endl;
        printUsage(argv[0]);
        return 1;
    }
    
    return 0;
}
