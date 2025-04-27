#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include "../include/mfp_base.h"
#include "../include/mfp_method1.h"
#include "../include/mfp_method2.h"
#include "../include/mfp_method3.h"

// Test function to measure factorization time
void testFactorization(mfp::MFPBase* method, const std::string& number, const std::string& methodName) {
    std::cout << "Testing " << methodName << " with number: " << number << std::endl;
    
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<std::string> factors = method->factorize(number);
    auto end = std::chrono::high_resolution_clock::now();
    
    std::chrono::duration<double> elapsed = end - start;
    
    std::cout << "Factors: ";
    for (const auto& factor : factors) {
        std::cout << factor << " ";
    }
    std::cout << std::endl;
    
    std::cout << "Time taken: " << elapsed.count() << " seconds" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
}

int main() {
    std::cout << "Testing MFP Method 3 with Search Space Reduction" << std::endl;
    std::cout << "================================================" << std::endl;
    
    // Create instances of all three methods for comparison
    mfp::MFPMethod1 method1;
    mfp::MFPMethod2 method2;
    mfp::MFPMethod3 method3(8); // Use 8 threads
    
    // Test cases for different system levels
    std::vector<std::pair<std::string, std::string>> testCases = {
        // System level 1-5 (small numbers)
        {"123456789", "System Level ~3"},
        {"1234567890123", "System Level ~5"},
        
        // System level 6-10 (medium numbers)
        {"12345678901234567", "System Level ~7"},
        {"1234567890123456789012", "System Level ~10"},
        
        // System level 11-15 (larger numbers)
        {"12345678901234567890123456789", "System Level ~13"},
        {"123456789012345678901234567890123456", "System Level ~15"},
        
        // System level 16-20 (very large numbers)
        {"1234567890123456789012345678901234567890", "System Level ~17"},
        {"12345678901234567890123456789012345678901234567890", "System Level ~20"}
    };
    
    // Known composite numbers with factors
    std::vector<std::string> compositeNumbers = {
        "123456789", // 3^2 * 3607 * 3803
        "9999999967", // 333667 * 29970
        "1000000007", // 47 * 751 * 28351
        "2147483647" // 2147483647 (prime)
    };
    
    // Test with known composite numbers
    std::cout << "Testing with known composite numbers:" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    
    for (const auto& number : compositeNumbers) {
        // Test with all three methods for comparison
        testFactorization(&method1, number, "Method 1 (Expanded q Factorization)");
        testFactorization(&method2, number, "Method 2 (Ultrafast with Structural Filter)");
        testFactorization(&method3, number, "Method 3 (Parallelized with Search Space Reduction)");
        std::cout << "========================================" << std::endl;
    }
    
    // Test with different system levels
    std::cout << "Testing with different system levels:" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    
    for (const auto& testCase : testCases) {
        std::cout << "Test case: " << testCase.second << std::endl;
        testFactorization(&method3, testCase.first, "Method 3 (Parallelized with Search Space Reduction)");
        std::cout << "========================================" << std::endl;
    }
    
    return 0;
}
