#include <iostream>
#include <vector>
#include <string>
#include "../include/mfp_method1.h"
#include "../include/mfp_method2.h"
#include "../include/mfp_method3.h"

int main() {
    // A composite number with known factors
    std::string composite = "1522605027922533";  // 1234567 × 1234567
    
    std::cout << "Testing MFP with known composite number: " << composite << std::endl;
    std::cout << "Expected factors: 1234567, 1234567" << std::endl;
    
    // Test Method 1
    std::cout << "\nTesting Method 1 (Expanded q Factorization):\n";
    mfp::MFPMethod1 method1;
    std::vector<std::string> factors1 = method1.factorize(composite);
    
    std::cout << "Factors found: ";
    for (size_t i = 0; i < factors1.size(); ++i) {
        std::cout << factors1[i];
        if (i < factors1.size() - 1) std::cout << ", ";
    }
    std::cout << std::endl;
    
    // Verify the factors by multiplying them
    mpz_t product, factor, expected;
    mpz_inits(product, factor, expected, nullptr);
    
    mpz_set_ui(product, 1);
    for (const auto& f : factors1) {
        mpz_set_str(factor, f.c_str(), 10);
        mpz_mul(product, product, factor);
    }
    
    mpz_set_str(expected, composite.c_str(), 10);
    
    if (mpz_cmp(product, expected) == 0) {
        std::cout << "Verification: PASSED - Factors multiply to give the original number" << std::endl;
    } else {
        std::cout << "Verification: FAILED - Factors do not multiply to give the original number" << std::endl;
    }
    
    // Test Method 2
    std::cout << "\nTesting Method 2 (Ultrafast with Structural Filter):\n";
    mfp::MFPMethod2 method2;
    std::vector<std::string> factors2 = method2.factorize(composite);
    
    std::cout << "Factors found: ";
    for (size_t i = 0; i < factors2.size(); ++i) {
        std::cout << factors2[i];
        if (i < factors2.size() - 1) std::cout << ", ";
    }
    std::cout << std::endl;
    
    // Verify the factors
    mpz_set_ui(product, 1);
    for (const auto& f : factors2) {
        mpz_set_str(factor, f.c_str(), 10);
        mpz_mul(product, product, factor);
    }
    
    if (mpz_cmp(product, expected) == 0) {
        std::cout << "Verification: PASSED - Factors multiply to give the original number" << std::endl;
    } else {
        std::cout << "Verification: FAILED - Factors do not multiply to give the original number" << std::endl;
    }
    
    // Test Method 3
    std::cout << "\nTesting Method 3 (Parallelized with Dynamic Blocks):\n";
    mfp::MFPMethod3 method3(8); // Use 8 threads
    std::vector<std::string> factors3 = method3.factorize(composite);
    
    std::cout << "Factors found: ";
    for (size_t i = 0; i < factors3.size(); ++i) {
        std::cout << factors3[i];
        if (i < factors3.size() - 1) std::cout << ", ";
    }
    std::cout << std::endl;
    
    // Verify the factors
    mpz_set_ui(product, 1);
    for (const auto& f : factors3) {
        mpz_set_str(factor, f.c_str(), 10);
        mpz_mul(product, product, factor);
    }
    
    if (mpz_cmp(product, expected) == 0) {
        std::cout << "Verification: PASSED - Factors multiply to give the original number" << std::endl;
    } else {
        std::cout << "Verification: FAILED - Factors do not multiply to give the original number" << std::endl;
    }
    
    mpz_clears(product, factor, expected, nullptr);
    
    // Test with another composite number
    std::string largeComposite = "9007199254740991";  // 6361 × 1416003655831
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Testing with larger composite number: " << largeComposite << std::endl;
    std::cout << "Expected factors: 6361, 1416003655831" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    // Test Method 3 with the larger number (using more threads)
    std::cout << "Testing Method 3 (Parallelized with Dynamic Blocks) with 28 threads:\n";
    mfp::MFPMethod3 method3Large(28); // Use 28 threads
    std::vector<std::string> factors3Large = method3Large.factorize(largeComposite);
    
    std::cout << "Factors found: ";
    for (size_t i = 0; i < factors3Large.size(); ++i) {
        std::cout << factors3Large[i];
        if (i < factors3Large.size() - 1) std::cout << ", ";
    }
    std::cout << std::endl;
    
    // Verify the factors
    mpz_inits(product, factor, expected, nullptr);
    mpz_set_ui(product, 1);
    for (const auto& f : factors3Large) {
        mpz_set_str(factor, f.c_str(), 10);
        mpz_mul(product, product, factor);
    }
    
    mpz_set_str(expected, largeComposite.c_str(), 10);
    
    if (mpz_cmp(product, expected) == 0) {
        std::cout << "Verification: PASSED - Factors multiply to give the original number" << std::endl;
    } else {
        std::cout << "Verification: FAILED - Factors do not multiply to give the original number" << std::endl;
    }
    
    mpz_clears(product, factor, expected, nullptr);
    
    return 0;
}
