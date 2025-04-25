#include "mfp_method1.h"
#include <chrono>
#include <gmp.h>
#include <vector>

namespace mfp {

MFPMethod1::MFPMethod1(int num_threads) : num_threads_(num_threads) {
    // Initialize method-specific parameters
}

MFPMethod1::~MFPMethod1() {
    // Clean up method-specific resources
}

bool MFPMethod1::isPrime(const mpz_t& number) {
    // Start timer if performance logging is enabled
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Check if number is small prime
    if (isSmallPrime(number)) {
        if (performance_logging_enabled_) {
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count() / 1000000.0;
            
            metrics_.isPrime_time += duration;
            metrics_.total_time += duration;
            metrics_.isPrime_calls++;
            metrics_.total_digits_processed += mpz_sizeinbase(number, 10);
        }
        return true;
    }
    
    // Try to find a divisor
    mpz_t divisor;
    mpz_init(divisor);
    
    bool has_divisor = findDivisor(number, divisor);
    
    mpz_clear(divisor);
    
    // If performance logging is enabled, update metrics
    if (performance_logging_enabled_) {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count() / 1000000.0;
        
        metrics_.isPrime_time += duration;
        metrics_.total_time += duration;
        metrics_.isPrime_calls++;
        metrics_.total_digits_processed += mpz_sizeinbase(number, 10);
    }
    
    return !has_divisor;
}

bool MFPMethod1::findDivisor(const mpz_t& number, mpz_t& divisor_out) {
    // Start timer if performance logging is enabled
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Check for small factors first
    if (hasSmallFactor(number, divisor_out)) {
        if (performance_logging_enabled_) {
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count() / 1000000.0;
            
            metrics_.findDivisor_time += duration;
            metrics_.total_time += duration;
            metrics_.findDivisor_calls++;
            metrics_.total_digits_processed += mpz_sizeinbase(number, 10);
        }
        return true;
    }
    
    // Apply expanded q factorization
    bool found = expandedQFactorization(number, divisor_out);
    
    // If performance logging is enabled, update metrics
    if (performance_logging_enabled_) {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count() / 1000000.0;
        
        metrics_.findDivisor_time += duration;
        metrics_.total_time += duration;
        metrics_.findDivisor_calls++;
        metrics_.total_digits_processed += mpz_sizeinbase(number, 10);
    }
    
    return found;
}

bool MFPMethod1::expandedQFactorization(const mpz_t& number, mpz_t& divisor_out) {
    // Compute q-expansion
    std::vector<mpz_t> q_values;
    if (!computeQExpansion(number, q_values)) {
        // Clean up q_values
        for (auto& q : q_values) {
            mpz_clear(q);
        }
        return false;
    }
    
    // Find pattern in q-sequence
    bool found = findPatternInQSequence(q_values, divisor_out);
    
    // Clean up q_values
    for (auto& q : q_values) {
        mpz_clear(q);
    }
    
    return found;
}

bool MFPMethod1::computeQExpansion(const mpz_t& number, std::vector<mpz_t>& q_values) {
    // Initialize q_0 = 1
    q_values.emplace_back();
    mpz_init_set_ui(q_values.back(), 1);
    
    // Initialize q_1 = number - 1
    q_values.emplace_back();
    mpz_init(q_values.back());
    mpz_sub_ui(q_values.back(), number, 1);
    
    // Compute q_i = number * q_{i-1} - q_{i-2}
    for (int i = 2; i <= 100; i++) {
        q_values.emplace_back();
        mpz_init(q_values.back());
        
        // q_i = number * q_{i-1} - q_{i-2}
        mpz_mul(q_values.back(), number, q_values[i-1]);
        mpz_sub(q_values.back(), q_values.back(), q_values[i-2]);
        
        // Check for overflow or other issues
        if (mpz_sgn(q_values.back()) < 0) {
            return false;
        }
    }
    
    return true;
}

bool MFPMethod1::findPatternInQSequence(const std::vector<mpz_t>& q_values, mpz_t& divisor_out) {
    // Look for patterns in the q-sequence that indicate divisibility
    
    // Check for common divisors between consecutive q-values
    for (size_t i = 1; i < q_values.size(); i++) {
        mpz_t gcd;
        mpz_init(gcd);
        
        mpz_gcd(gcd, q_values[i], q_values[i-1]);
        
        if (mpz_cmp_ui(gcd, 1) > 0) {
            mpz_set(divisor_out, gcd);
            mpz_clear(gcd);
            return true;
        }
        
        mpz_clear(gcd);
    }
    
    // Check for periodicity in the sequence modulo small primes
    const unsigned int small_primes[] = {101, 103, 107, 109, 113, 127, 131, 137, 139, 149};
    
    for (unsigned int prime : small_primes) {
        std::vector<unsigned int> remainders;
        
        for (const auto& q : q_values) {
            remainders.push_back(mpz_fdiv_ui(q, prime));
        }
        
        // Look for a repeating pattern
        for (size_t period = 1; period <= remainders.size() / 2; period++) {
            bool is_periodic = true;
            
            for (size_t i = remainders.size() - period; i < remainders.size(); i++) {
                if (remainders[i] != remainders[i - period]) {
                    is_periodic = false;
                    break;
                }
            }
            
            if (is_periodic) {
                // Found a potential divisor
                mpz_t potential_divisor;
                mpz_init(potential_divisor);
                
                // Compute the potential divisor based on the pattern
                mpz_ui_pow_ui(potential_divisor, prime, period);
                
                // Verify that it's actually a divisor
                if (mpz_divisible_p(q_values.back(), potential_divisor)) {
                    mpz_set(divisor_out, potential_divisor);
                    mpz_clear(potential_divisor);
                    return true;
                }
                
                mpz_clear(potential_divisor);
            }
        }
    }
    
    return false;
}

} // namespace mfp
