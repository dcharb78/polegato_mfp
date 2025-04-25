#include "mfp_method1.h"
#include <gmp.h>
#include <iostream>
#include <cmath>

namespace mfp {

MFPMethod1::MFPMethod1() {
    // Initialize Method 1 (Expanded q Factorization)
}

MFPMethod1::~MFPMethod1() {
    // Nothing to clean up
}

bool MFPMethod1::isPrime(const std::string& number) {
    // For Method 1, we'll use the base class implementation
    // which uses Miller-Rabin for large numbers
    return MFPBase::isPrime(number);
}

std::vector<std::string> MFPMethod1::factorize(const std::string& number) {
    std::vector<std::string> factors;
    
    // If the number is prime, just return it
    if (isPrime(number)) {
        factors.push_back(number);
        return factors;
    }
    
    // Use the Expanded q Factorization method
    if (expandedQFactorization(number, factors)) {
        return factors;
    }
    
    // Fallback to trial division for small numbers
    try {
        unsigned long n = std::stoul(number);
        if (n <= 1000000) {
            // Trial division for small numbers
            if (n <= 1) {
                return factors; // Empty for 0 and 1
            }
            
            while (n % 2 == 0) {
                factors.push_back("2");
                n /= 2;
            }
            
            for (unsigned long i = 3; i * i <= n; i += 2) {
                while (n % i == 0) {
                    factors.push_back(std::to_string(i));
                    n /= i;
                }
            }
            
            if (n > 1) {
                factors.push_back(std::to_string(n));
            }
            
            return factors;
        }
    } catch (const std::exception& e) {
        // Number is too large for unsigned long, continue with GMP
    }
    
    // If all else fails, just return the number itself
    factors.push_back(number);
    return factors;
}

std::string MFPMethod1::findNextPrime(const std::string& number) {
    // Use the base class implementation
    return MFPBase::findNextPrime(number);
}

bool MFPMethod1::expandedQFactorization(const std::string& number, std::vector<std::string>& factors) {
    mpz_t n, q, a, b, gcd;
    mpz_init(n);
    mpz_init(q);
    mpz_init(a);
    mpz_init(b);
    mpz_init(gcd);
    
    // Convert string to mpz_t
    mpz_set_str(n, number.c_str(), 10);
    
    // Check if n is even
    if (mpz_even_p(n) != 0) {
        factors.push_back("2");
        mpz_divexact_ui(n, n, 2);
        
        // Convert n back to string and recursively factorize
        char* n_str = mpz_get_str(nullptr, 10, n);
        std::string remaining(n_str);
        free(n_str);
        
        std::vector<std::string> remaining_factors = factorize(remaining);
        factors.insert(factors.end(), remaining_factors.begin(), remaining_factors.end());
        
        mpz_clear(n);
        mpz_clear(q);
        mpz_clear(a);
        mpz_clear(b);
        mpz_clear(gcd);
        
        return true;
    }
    
    // Try to find q such that n = a^2 - b^2 = (a+b)(a-b)
    mpz_sqrt(q, n);
    mpz_add_ui(q, q, 1); // Start with q = sqrt(n) + 1
    
    // Try different values of q
    for (int i = 0; i < 1000; i++) {
        // Calculate a^2 = q^2 - n
        mpz_mul(a, q, q);
        mpz_sub(a, a, n);
        
        // Check if a is a perfect square
        if (mpz_perfect_square_p(a) != 0) {
            // Found a factorization
            mpz_sqrt(a, a); // a = sqrt(q^2 - n)
            
            // b = q - a
            mpz_sub(b, q, a);
            
            // First factor = q + a
            mpz_add(gcd, q, a);
            char* factor1 = mpz_get_str(nullptr, 10, gcd);
            factors.push_back(std::string(factor1));
            free(factor1);
            
            // Second factor = q - a
            char* factor2 = mpz_get_str(nullptr, 10, b);
            factors.push_back(std::string(factor2));
            free(factor2);
            
            mpz_clear(n);
            mpz_clear(q);
            mpz_clear(a);
            mpz_clear(b);
            mpz_clear(gcd);
            
            return true;
        }
        
        // Try next q
        mpz_add_ui(q, q, 1);
    }
    
    // If we get here, the method failed to find factors
    mpz_clear(n);
    mpz_clear(q);
    mpz_clear(a);
    mpz_clear(b);
    mpz_clear(gcd);
    
    return false;
}

} // namespace mfp
