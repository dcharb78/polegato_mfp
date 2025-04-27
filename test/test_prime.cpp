#include <iostream>
#include <vector>
#include <string>
#include "../include/mfp_method1.h"
#include "../include/mfp_method2.h"
#include "../include/mfp_method3.h"

int main() {
    // A large prime number
    std::string prime = "618970019642690137449562111";
    
    std::cout << "Testing MFP with known prime: " << prime << std::endl;
    
    // Test Method 1
    std::cout << "\nTesting Method 1 (Expanded q Factorization):\n";
    mfp::MFPMethod1 method1;
    std::vector<std::string> factors1 = method1.factorize(prime);
    
    std::cout << "Factors found: ";
    for (size_t i = 0; i < factors1.size(); ++i) {
        std::cout << factors1[i];
        if (i < factors1.size() - 1) std::cout << ", ";
    }
    std::cout << std::endl;
    
    // Verify that only one factor was found (the prime itself)
    if (factors1.size() == 1 && factors1[0] == prime) {
        std::cout << "Verification: PASSED - Correctly identified as prime" << std::endl;
    } else {
        std::cout << "Verification: FAILED - Did not correctly identify as prime" << std::endl;
    }
    
    // Test Method 2
    std::cout << "\nTesting Method 2 (Ultrafast with Structural Filter):\n";
    mfp::MFPMethod2 method2;
    std::vector<std::string> factors2 = method2.factorize(prime);
    
    std::cout << "Factors found: ";
    for (size_t i = 0; i < factors2.size(); ++i) {
        std::cout << factors2[i];
        if (i < factors2.size() - 1) std::cout << ", ";
    }
    std::cout << std::endl;
    
    // Verify that only one factor was found (the prime itself)
    if (factors2.size() == 1 && factors2[0] == prime) {
        std::cout << "Verification: PASSED - Correctly identified as prime" << std::endl;
    } else {
        std::cout << "Verification: FAILED - Did not correctly identify as prime" << std::endl;
    }
    
    // Test Method 3
    std::cout << "\nTesting Method 3 (Parallelized with Dynamic Blocks):\n";
    mfp::MFPMethod3 method3(8); // Use 8 threads
    std::vector<std::string> factors3 = method3.factorize(prime);
    
    std::cout << "Factors found: ";
    for (size_t i = 0; i < factors3.size(); ++i) {
        std::cout << factors3[i];
        if (i < factors3.size() - 1) std::cout << ", ";
    }
    std::cout << std::endl;
    
    // Verify that only one factor was found (the prime itself)
    if (factors3.size() == 1 && factors3[0] == prime) {
        std::cout << "Verification: PASSED - Correctly identified as prime" << std::endl;
    } else {
        std::cout << "Verification: FAILED - Did not correctly identify as prime" << std::endl;
    }
    
    return 0;
}
