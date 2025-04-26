#include "../include/mfp_method2.h"
#include <iostream>
#include <cmath>

namespace mfp {

MFPMethod2::MFPMethod2() {
    // Initialize Method 2 (Ultrafast with Structural Filter)
}

MFPMethod2::~MFPMethod2() {
    // Clean up resources
}

bool MFPMethod2::isPrime(const std::string& number) {
    // For small numbers, use the base class implementation
    try {
        unsigned long n = std::stoul(number);
        if (n < 1000000) {
            return MFPBase::isPrime(number);
        }
    } catch (const std::exception& e) {
        // Number is too large for unsigned long, continue with MFP approach
    }
    
    // For larger numbers, try to find a divisor using the MFP approach
    mpz_t n, divisor;
    mpz_inits(n, divisor, nullptr);
    
    // Convert string to mpz_t
    strToMpz(number, n);
    
    // Check if n is divisible by small primes
    if (checkSmallPrimes(n, divisor)) {
        mpz_clears(n, divisor, nullptr);
        return false;
    }
    
    // Try to find a divisor using the Ultrafast with Structural Filter method
    bool hasDivisor = ultrafastFactorization(n, divisor);
    
    mpz_clears(n, divisor, nullptr);
    
    // If we found a divisor, the number is not prime
    return !hasDivisor;
}

std::vector<std::string> MFPMethod2::factorize(const std::string& number) {
    std::vector<std::string> factors;
    
    // If the number is prime, just return it
    if (isPrime(number)) {
        factors.push_back(number);
        return factors;
    }
    
    mpz_t n, divisor, quotient;
    mpz_inits(n, divisor, quotient, nullptr);
    
    // Convert string to mpz_t
    strToMpz(number, n);
    
    // Check if n is divisible by small primes
    if (checkSmallPrimes(n, divisor)) {
        // Add the small prime to the factors
        factors.push_back(mpzToStr(divisor));
        
        // Calculate the quotient
        mpz_divexact(quotient, n, divisor);
        
        // Recursively factorize the quotient
        std::vector<std::string> remaining_factors = factorize(mpzToStr(quotient));
        factors.insert(factors.end(), remaining_factors.begin(), remaining_factors.end());
        
        mpz_clears(n, divisor, quotient, nullptr);
        return factors;
    }
    
    // Try to find a divisor using the Ultrafast with Structural Filter method
    if (ultrafastFactorization(n, divisor)) {
        // Add the divisor to the factors
        factors.push_back(mpzToStr(divisor));
        
        // Calculate the quotient
        mpz_divexact(quotient, n, divisor);
        
        // Recursively factorize the quotient
        std::vector<std::string> remaining_factors = factorize(mpzToStr(quotient));
        factors.insert(factors.end(), remaining_factors.begin(), remaining_factors.end());
        
        mpz_clears(n, divisor, quotient, nullptr);
        return factors;
    }
    
    // If we get here, we couldn't find any factors, so the number must be prime
    factors.push_back(number);
    
    mpz_clears(n, divisor, quotient, nullptr);
    return factors;
}

bool MFPMethod2::ultrafastFactorization(const mpz_t n, mpz_t divisor) {
    // Try each multiplier k in {1, 3, 7, 9}
    const int ks[] = {1, 3, 7, 9};
    
    for (int k : ks) {
        if (testUltrafastDivisor(n, k, divisor)) {
            return true;
        }
    }
    
    return false;
}

bool MFPMethod2::testUltrafastDivisor(const mpz_t n, int k, mpz_t divisor) {
    mpz_t nk, A, sqrtA;
    mpz_inits(nk, A, sqrtA, nullptr);
    
    // Calculate nk = n * k
    mpz_mul_ui(nk, n, k);
    
    // Calculate A = floor(nk / 10)
    mpz_fdiv_q_ui(A, nk, 10);
    
    // Calculate d0 = nk % 10
    unsigned long d0 = mpz_fdiv_ui(nk, 10);
    
    // Calculate q_max = 2 * sqrt(A)
    mpz_sqrt(sqrtA, A);
    unsigned long q_max = mpz_get_ui(sqrtA) * 2;
    
    // Get A as unsigned long for calculations
    unsigned long A_ul = mpz_get_ui(A);
    
    // Try each q from 1 to q_max
    for (unsigned long q = 1; q <= q_max; ++q) {
        // Calculate denom = 10*q + 1
        unsigned long denom = 10 * q + 1;
        
        // Calculate qd0 = q * d0
        unsigned long qd0 = q * d0;
        
        // Skip if qd0 > A
        if (qd0 > A_ul) continue;
        
        // Calculate numer = A - qd0
        unsigned long numer_ul = A_ul - qd0;
        
        // Check if i is an integer (numer is divisible by denom)
        if (numer_ul % denom != 0) continue;
        
        // Calculate i = numer / denom
        unsigned long i = numer_ul / denom;
        
        // Calculate d = d0 + 10*i
        unsigned long d = d0 + 10 * i;
        
        // Skip if d <= 1
        if (d <= 1) continue;
        
        // Additional structural filter: Check if (A - i) % d == 0
        unsigned long Ai_ul = A_ul - i;
        if (Ai_ul % d != 0) continue;
        
        // Check if n is divisible by d
        if (!mpz_divisible_ui_p(n, d)) continue;
        
        // Found a divisor
        mpz_set_ui(divisor, d);
        mpz_clears(nk, A, sqrtA, nullptr);
        return true;
    }
    
    // No divisor found
    mpz_clears(nk, A, sqrtA, nullptr);
    return false;
}

} // namespace mfp
