#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include "../include/mfp_method1.h"
#include "../include/mfp_method2.h"
#include "../include/mfp_method3.h"

int main() {
    // A composite number with known factors
    std::string composite = "1522605027922533";  // 1234567 × 1234567
    
    std::cout << "Testing MFP with composite number: " << composite << std::endl;
    std::cout << "Expected factors: 1234567, 1234567" << std::endl;
    
    // Test Method 1
    std::cout << "\nTesting Method 1 (Expanded q Factorization):\n";
    mfp::MFPMethod1 method1;
    
    auto t0 = std::chrono::high_resolution_clock::now();
    std::vector<std::string> factors1 = method1.factorize(composite);
    auto t1 = std::chrono::high_resolution_clock::now();
    
    std::cout << "Factors found: ";
    for (size_t i = 0; i < factors1.size(); ++i) {
        std::cout << factors1[i];
        if (i < factors1.size() - 1) std::cout << ", ";
    }
    std::cout << std::endl;
    
    double dt1 = std::chrono::duration<double>(t1 - t0).count();
    std::cout << "Time: " << dt1 << " s\n";
    
    // Test Method 2
    std::cout << "\nTesting Method 2 (Ultrafast with Structural Filter):\n";
    mfp::MFPMethod2 method2;
    
    t0 = std::chrono::high_resolution_clock::now();
    std::vector<std::string> factors2 = method2.factorize(composite);
    t1 = std::chrono::high_resolution_clock::now();
    
    std::cout << "Factors found: ";
    for (size_t i = 0; i < factors2.size(); ++i) {
        std::cout << factors2[i];
        if (i < factors2.size() - 1) std::cout << ", ";
    }
    std::cout << std::endl;
    
    double dt2 = std::chrono::duration<double>(t1 - t0).count();
    std::cout << "Time: " << dt2 << " s\n";
    
    // Test Method 3
    std::cout << "\nTesting Method 3 (Parallelized with Dynamic Blocks):\n";
    mfp::MFPMethod3 method3(8); // Use 8 threads
    
    t0 = std::chrono::high_resolution_clock::now();
    std::vector<std::string> factors3 = method3.factorize(composite);
    t1 = std::chrono::high_resolution_clock::now();
    
    std::cout << "Factors found: ";
    for (size_t i = 0; i < factors3.size(); ++i) {
        std::cout << factors3[i];
        if (i < factors3.size() - 1) std::cout << ", ";
    }
    std::cout << std::endl;
    
    double dt3 = std::chrono::duration<double>(t1 - t0).count();
    std::cout << "Time: " << dt3 << " s\n";
    
    return 0;
}
