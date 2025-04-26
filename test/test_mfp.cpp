#include <iostream>
#include <vector>
#include <string>
#include "../include/mfp_method1.h"
#include "../include/mfp_method2.h"
#include "../include/mfp_method3.h"

int main() {
    std::cout << "MFP Implementation Test Suite\n";
    std::cout << "============================\n\n";
    
    // Test numbers from the PDF
    std::vector<std::string> numbers = {
        "91",
        "15",
        "2199023255551",
        "9007199254740991",
        "147573952589676412927"
    };

    // Test Method 1: Expanded q Factorization
    std::cout << "=== Testing Method 1: Expanded q Factorization ===\n";
    mfp::MFPMethod1 method1;
    
    for (const std::string &snum : numbers) {
        std::cout << "\nNumber: " << snum << "\n";
        
        auto t0 = std::chrono::high_resolution_clock::now();
        std::vector<std::string> factors = method1.factorize(snum);
        auto t1 = std::chrono::high_resolution_clock::now();
        
        if (factors.size() == 1 && factors[0] == snum) {
            std::cout << " No divisor (prime)\n";
        } else {
            std::cout << " Factors: ";
            for (size_t i = 0; i < factors.size(); ++i) {
                std::cout << factors[i];
                if (i < factors.size() - 1) std::cout << ", ";
            }
            std::cout << "\n";
        }
        
        double dt = std::chrono::duration<double>(t1 - t0).count();
        std::cout << " Time: " << dt << " s\n";
        std::cout << "-----------------------------\n";
    }
    
    // Test Method 2: Ultrafast with Structural Filter
    std::cout << "\n=== Testing Method 2: Ultrafast with Structural Filter ===\n";
    mfp::MFPMethod2 method2;
    
    for (const std::string &snum : numbers) {
        std::cout << "\nNumber: " << snum << "\n";
        
        auto t0 = std::chrono::high_resolution_clock::now();
        std::vector<std::string> factors = method2.factorize(snum);
        auto t1 = std::chrono::high_resolution_clock::now();
        
        if (factors.size() == 1 && factors[0] == snum) {
            std::cout << " No divisor (prime)\n";
        } else {
            std::cout << " Factors: ";
            for (size_t i = 0; i < factors.size(); ++i) {
                std::cout << factors[i];
                if (i < factors.size() - 1) std::cout << ", ";
            }
            std::cout << "\n";
        }
        
        double dt = std::chrono::duration<double>(t1 - t0).count();
        std::cout << " Time: " << dt << " s\n";
        std::cout << "-----------------------------\n";
    }
    
    // Test Method 3: Parallelized with Dynamic Blocks
    std::cout << "\n=== Testing Method 3: Parallelized with Dynamic Blocks ===\n";
    mfp::MFPMethod3 method3(8); // Use 8 threads as specified in the PDF
    
    for (const std::string &snum : numbers) {
        std::cout << "\nNumber: " << snum << "\n";
        
        auto t0 = std::chrono::high_resolution_clock::now();
        std::vector<std::string> factors = method3.factorize(snum);
        auto t1 = std::chrono::high_resolution_clock::now();
        
        if (factors.size() == 1 && factors[0] == snum) {
            std::cout << " No divisor (prime)\n";
        } else {
            std::cout << " Factors: ";
            for (size_t i = 0; i < factors.size(); ++i) {
                std::cout << factors[i];
                if (i < factors.size() - 1) std::cout << ", ";
            }
            std::cout << "\n";
        }
        
        double dt = std::chrono::duration<double>(t1 - t0).count();
        std::cout << " Time: " << dt << " s\n";
        std::cout << "-----------------------------\n";
    }
    
    return 0;
}
