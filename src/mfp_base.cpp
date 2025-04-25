#include "mfp_base.h"
#include <gmp.h>
#include <vector>
#include <chrono>
#include <algorithm>

namespace mfp {

MFPBase::MFPBase() : performance_logging_enabled_(false) {
    // Initialize performance metrics
    metrics_.total_time = 0.0;
    metrics_.isPrime_time = 0.0;
    metrics_.findDivisor_time = 0.0;
    metrics_.isPrime_calls = 0;
    metrics_.findDivisor_calls = 0;
    metrics_.total_digits_processed = 0;
}

MFPBase::~MFPBase() {
    // Nothing to clean up
}

bool MFPBase::factorize(const mpz_t& number, std::vector<mpz_t>& factors_out) {
    // Clear output vector
    for (auto& factor : factors_out) {
        mpz_clear(factor);
    }
    factors_out.clear();
    
    // Check if number is 0 or 1
    if (mpz_cmp_ui(number, 1) <= 0) {
        return true;
    }
    
    // Make a copy of the number to work with
    mpz_t n;
    mpz_init_set(n, number);
    
    // Check if the number is already prime
    if (isPrime(n)) {
        mpz_t factor;
        mpz_init_set(factor, n);
        factors_out.push_back(factor);
        mpz_clear(n);
        return true;
    }
    
    // Find a divisor
    mpz_t divisor;
    mpz_init(divisor);
    
    if (!findDivisor(n, divisor)) {
        mpz_clear(divisor);
        mpz_clear(n);
        return false;
    }
    
    // Calculate the quotient
    mpz_t quotient;
    mpz_init(quotient);
    mpz_divexact(quotient, n, divisor);
    
    // Recursively factorize the divisor and quotient
    std::vector<mpz_t> divisor_factors;
    std::vector<mpz_t> quotient_factors;
    
    bool success = factorize(divisor, divisor_factors) && factorize(quotient, quotient_factors);
    
    if (success) {
        // Combine the factors
        factors_out.insert(factors_out.end(), divisor_factors.begin(), divisor_factors.end());
        factors_out.insert(factors_out.end(), quotient_factors.begin(), quotient_factors.end());
        
        // Sort factors in ascending order
        std::sort(factors_out.begin(), factors_out.end(), 
                 [](const mpz_t& a, const mpz_t& b) { return mpz_cmp(a, b) < 0; });
    }
    
    // Clean up
    mpz_clear(divisor);
    mpz_clear(quotient);
    mpz_clear(n);
    
    return success;
}

void MFPBase::setPerformanceLogging(bool enable) {
    performance_logging_enabled_ = enable;
    
    if (!enable) {
        // Reset metrics when disabling
        metrics_.total_time = 0.0;
        metrics_.isPrime_time = 0.0;
        metrics_.findDivisor_time = 0.0;
        metrics_.isPrime_calls = 0;
        metrics_.findDivisor_calls = 0;
        metrics_.total_digits_processed = 0;
    }
}

MFPBase::PerformanceMetrics MFPBase::getMetrics() const {
    return metrics_;
}

bool MFPBase::isSmallPrime(const mpz_t& number) {
    // Check if number is 2 or 3
    if (mpz_cmp_ui(number, 2) == 0 || mpz_cmp_ui(number, 3) == 0) {
        return true;
    }
    
    // Check if number is less than 2 or divisible by 2
    if (mpz_cmp_ui(number, 2) < 0 || mpz_even_p(number)) {
        return false;
    }
    
    // Check divisibility by small primes
    const unsigned int small_primes[] = {3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97};
    
    for (unsigned int prime : small_primes) {
        if (mpz_cmp_ui(number, prime) == 0) {
            return true;
        }
        
        if (mpz_divisible_ui_p(number, prime)) {
            return false;
        }
    }
    
    // For larger numbers, use Miller-Rabin test
    if (mpz_cmp_ui(number, 100) > 0) {
        return millerRabinTest(number, 25);
    }
    
    // For numbers between 97 and 100, do trial division
    mpz_t i, sqrt_n;
    mpz_init_set_ui(i, 101);
    mpz_init(sqrt_n);
    mpz_sqrt(sqrt_n, number);
    
    bool is_prime = true;
    
    while (mpz_cmp(i, sqrt_n) <= 0) {
        if (mpz_divisible_p(number, i)) {
            is_prime = false;
            break;
        }
        mpz_add_ui(i, i, 2);
    }
    
    mpz_clear(i);
    mpz_clear(sqrt_n);
    
    return is_prime;
}

bool MFPBase::hasSmallFactor(const mpz_t& number, mpz_t& factor_out) {
    // Check if divisible by 2
    if (mpz_even_p(number)) {
        mpz_set_ui(factor_out, 2);
        return true;
    }
    
    // Check divisibility by small primes
    const unsigned int small_primes[] = {3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97};
    
    for (unsigned int prime : small_primes) {
        if (mpz_divisible_ui_p(number, prime)) {
            mpz_set_ui(factor_out, prime);
            return true;
        }
    }
    
    return false;
}

bool MFPBase::millerRabinTest(const mpz_t& number, int iterations) {
    // Miller-Rabin primality test
    // Returns true if number is probably prime, false if definitely composite
    
    // Check small cases
    if (mpz_cmp_ui(number, 2) == 0 || mpz_cmp_ui(number, 3) == 0) {
        return true;
    }
    
    if (mpz_cmp_ui(number, 2) < 0 || mpz_even_p(number)) {
        return false;
    }
    
    // Write n-1 as d*2^s by factoring out powers of 2
    mpz_t d, n_minus_1;
    mpz_init(d);
    mpz_init(n_minus_1);
    
    mpz_sub_ui(n_minus_1, number, 1);
    mpz_set(d, n_minus_1);
    
    unsigned int s = 0;
    while (mpz_even_p(d)) {
        mpz_divexact_ui(d, d, 2);
        s++;
    }
    
    // Witness loop
    gmp_randstate_t rng;
    gmp_randinit_default(rng);
    gmp_randseed_ui(rng, time(nullptr));
    
    mpz_t a, x, y;
    mpz_init(a);
    mpz_init(x);
    mpz_init(y);
    
    bool probably_prime = true;
    
    for (int i = 0; i < iterations && probably_prime; i++) {
        // Generate random base a in [2, n-2]
        do {
            mpz_urandomm(a, rng, n_minus_1);
        } while (mpz_cmp_ui(a, 1) <= 0);
        
        // Compute x = a^d mod n
        mpz_powm(x, a, d, number);
        
        // If x = 1 or x = n-1, skip to next iteration
        if (mpz_cmp_ui(x, 1) == 0 || mpz_cmp(x, n_minus_1) == 0) {
            continue;
        }
        
        // Repeat s-1 times
        bool composite = true;
        for (unsigned int j = 0; j < s - 1; j++) {
            // y = x^2 mod n
            mpz_powm_ui(y, x, 2, number);
            
            // If y = 1, x is a non-trivial square root of 1, so n is composite
            if (mpz_cmp_ui(y, 1) == 0) {
                probably_prime = false;
                break;
            }
            
            // If y = n-1, n might be prime, so continue with next witness
            if (mpz_cmp(y, n_minus_1) == 0) {
                composite = false;
                break;
            }
            
            mpz_set(x, y);
        }
        
        if (composite) {
            probably_prime = false;
        }
    }
    
    // Clean up
    mpz_clear(d);
    mpz_clear(n_minus_1);
    mpz_clear(a);
    mpz_clear(x);
    mpz_clear(y);
    gmp_randclear(rng);
    
    return probably_prime;
}

std::unique_ptr<MFPBase> createMFPImplementation(int method_number, int num_threads) {
    switch (method_number) {
        case 1:
            return std::make_unique<MFPMethod1>(num_threads);
        case 2:
            return std::make_unique<MFPMethod2>(num_threads);
        case 3:
            return std::make_unique<MFPMethod3>(num_threads);
        default:
            // Default to Method 3
            return std::make_unique<MFPMethod3>(num_threads);
    }
}

} // namespace mfp
