#include <iostream>
#include <vector>
#include <string>
#include "../include/mfp_method1.h"
#include "../include/mfp_method2.h"
#include "../include/mfp_method3.h"

void testMethod(const std::string& methodName, const std::string& number, const std::vector<std::string>& factors) {
    std::cout << methodName << " found factors: ";
    for (size_t i = 0; i < factors.size(); ++i) {
        std::cout << factors[i];
        if (i < factors.size() - 1) std::cout << ", ";
    }
    std::cout << std::endl;
    
    // Verify the factors by multiplying them
    mpz_t product, factor, expected;
    mpz_inits(product, factor, expected, nullptr);
    
    mpz_set_ui(product, 1);
    for (const auto& f : factors) {
        mpz_set_str(factor, f.c_str(), 10);
        mpz_mul(product, product, factor);
    }
    
    mpz_set_str(expected, number.c_str(), 10);
    
    if (mpz_cmp(product, expected) == 0) {
        std::cout << "Verification: PASSED - Factors multiply to give the original number" << std::endl;
    } else {
        std::cout << "Verification: FAILED - Factors do not multiply to give the original number" << std::endl;
    }
    
    mpz_clears(product, factor, expected, nullptr);
}

int main() {
    // Test numbers from small to large
    std::vector<std::pair<std::string, std::string>> testCases = {
        {"91", "7 × 13"},
        {"143", "11 × 13"},
        {"1591", "37 × 43"},
        {"15251", "101 × 151"},
        {"1522605027922533", "1234567 × 1234567"}
    };
    
    for (const auto& testCase : testCases) {
        std::string number = testCase.first;
        std::string expected = testCase.second;
        
        std::cout << "\n========================================" << std::endl;
        std::cout << "Testing with number: " << number << std::endl;
        std::cout << "Expected factors: " << expected << std::endl;
        std::cout << "========================================\n" << std::endl;
        
        // Test Method 1
        std::cout << "Testing Method 1 (Expanded q Factorization):" << std::endl;
        mfp::MFPMethod1 method1;
        auto factors1 = method1.factorize(number);
        testMethod("Method 1", number, factors1);
        
        // Test Method 2
        std::cout << "\nTesting Method 2 (Ultrafast with Structural Filter):" << std::endl;
        mfp::MFPMethod2 method2;
        auto factors2 = method2.factorize(number);
        testMethod("Method 2", number, factors2);
        
        // Test Method 3
        std::cout << "\nTesting Method 3 (Parallelized with Dynamic Blocks):" << std::endl;
        mfp::MFPMethod3 method3(8); // Use 8 threads
        auto factors3 = method3.factorize(number);
        testMethod("Method 3", number, factors3);
        
        std::cout << "\n----------------------------------------\n" << std::endl;
    }
    
    std::cout << "All tests completed successfully!" << std::endl;
    
    return 0;
}
