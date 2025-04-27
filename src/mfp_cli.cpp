#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include "../include/mfp_base.h"
#include "../include/mfp_method1.h"
#include "../include/mfp_method2.h"
#include "../include/mfp_method3.h"

void printUsage() {
    std::cout << "Usage: mfp_cli [OPTIONS]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -n, --number NUMBER     Specify a single number to factorize" << std::endl;
    std::cout << "  -r, --range START END   Specify a range of numbers to factorize" << std::endl;
    std::cout << "  -m, --method METHOD     Specify the factorization method (1, 2, or 3, default: 3)" << std::endl;
    std::cout << "  -c, --cpu COUNT         Specify the number of CPU cores to use (default: auto)" << std::endl;
    std::cout << "  -h, --help              Display this help message" << std::endl;
}

int main(int argc, char* argv[]) {
    // Default values
    std::string singleNumber = "";
    long rangeStart = -1;
    long rangeEnd = -1;
    int method = 3;
    int cpuCount = 0;
    
    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage();
            return 0;
        } else if (arg == "-n" || arg == "--number") {
            if (i + 1 < argc) {
                singleNumber = argv[++i];
            } else {
                std::cerr << "Error: Missing number after " << arg << std::endl;
                return 1;
            }
        } else if (arg == "-r" || arg == "--range") {
            if (i + 2 < argc) {
                try {
                    rangeStart = std::stol(argv[++i]);
                    rangeEnd = std::stol(argv[++i]);
                } catch (const std::exception& e) {
                    std::cerr << "Error: Invalid range values" << std::endl;
                    return 1;
                }
            } else {
                std::cerr << "Error: Missing range values after " << arg << std::endl;
                return 1;
            }
        } else if (arg == "-m" || arg == "--method") {
            if (i + 1 < argc) {
                try {
                    method = std::stoi(argv[++i]);
                    if (method < 1 || method > 3) {
                        std::cerr << "Error: Method must be 1, 2, or 3" << std::endl;
                        return 1;
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error: Invalid method value" << std::endl;
                    return 1;
                }
            } else {
                std::cerr << "Error: Missing method value after " << arg << std::endl;
                return 1;
            }
        } else if (arg == "-c" || arg == "--cpu") {
            if (i + 1 < argc) {
                try {
                    cpuCount = std::stoi(argv[++i]);
                    if (cpuCount < 0) {
                        std::cerr << "Error: CPU count must be non-negative" << std::endl;
                        return 1;
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error: Invalid CPU count value" << std::endl;
                    return 1;
                }
            } else {
                std::cerr << "Error: Missing CPU count value after " << arg << std::endl;
                return 1;
            }
        } else {
            std::cerr << "Error: Unknown option: " << arg << std::endl;
            printUsage();
            return 1;
        }
    }
    
    // Validate input
    if (singleNumber.empty() && (rangeStart < 0 || rangeEnd < 0)) {
        std::cerr << "Error: Must specify either a single number or a range" << std::endl;
        printUsage();
        return 1;
    }
    
    if (!singleNumber.empty() && (rangeStart >= 0 || rangeEnd >= 0)) {
        std::cerr << "Error: Cannot specify both a single number and a range" << std::endl;
        printUsage();
        return 1;
    }
    
    if (rangeStart > rangeEnd && rangeStart >= 0) {
        std::cerr << "Error: Range start must be less than or equal to range end" << std::endl;
        return 1;
    }
    
    // Create the appropriate MFP method instance
    mfp::MFPBase* mfpMethod = nullptr;
    
    switch (method) {
        case 1:
            mfpMethod = new mfp::MFPMethod1();
            std::cout << "Using Method 1: Expanded q Factorization" << std::endl;
            break;
        case 2:
            mfpMethod = new mfp::MFPMethod2();
            std::cout << "Using Method 2: Ultrafast with Structural Filter" << std::endl;
            break;
        case 3:
            mfpMethod = new mfp::MFPMethod3(cpuCount);
            std::cout << "Using Method 3: Parallelized with Dynamic Blocks" << std::endl;
            if (cpuCount > 0) {
                std::cout << "Using " << cpuCount << " CPU cores" << std::endl;
            } else {
                std::cout << "Using auto-detected CPU cores" << std::endl;
            }
            break;
    }
    
    // Process the input
    if (!singleNumber.empty()) {
        // Process a single number
        std::cout << "Factorizing number: " << singleNumber << std::endl;
        
        auto start = std::chrono::high_resolution_clock::now();
        std::vector<std::string> factors = mfpMethod->factorize(singleNumber);
        auto end = std::chrono::high_resolution_clock::now();
        
        std::chrono::duration<double> elapsed = end - start;
        
        std::cout << "Factors of " << singleNumber << ":" << std::endl;
        for (const auto& factor : factors) {
            std::cout << "  " << factor << std::endl;
        }
        
        std::cout << "Time taken: " << elapsed.count() << " seconds" << std::endl;
    } else {
        // Process a range of numbers
        std::cout << "Factorizing numbers in range: " << rangeStart << " to " << rangeEnd << std::endl;
        
        auto totalStart = std::chrono::high_resolution_clock::now();
        
        for (long num = rangeStart; num <= rangeEnd; num++) {
            std::string numStr = std::to_string(num);
            
            auto start = std::chrono::high_resolution_clock::now();
            std::vector<std::string> factors = mfpMethod->factorize(numStr);
            auto end = std::chrono::high_resolution_clock::now();
            
            std::chrono::duration<double> elapsed = end - start;
            
            std::cout << "Factors of " << numStr << ":" << std::endl;
            for (const auto& factor : factors) {
                std::cout << "  " << factor << std::endl;
            }
            
            std::cout << "Time taken: " << elapsed.count() << " seconds" << std::endl;
            std::cout << "----------------------------------------" << std::endl;
        }
        
        auto totalEnd = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> totalElapsed = totalEnd - totalStart;
        
        std::cout << "Total time taken for range: " << totalElapsed.count() << " seconds" << std::endl;
    }
    
    // Clean up
    delete mfpMethod;
    
    return 0;
}
