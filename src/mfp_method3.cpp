#include "../include/mfp_method3.h"
#include <iostream>
#include <cmath>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <functional>

namespace mfp {

MFPMethod3::MFPMethod3(int numThreads) {
    // Initialize Method 3 (Parallelized with Dynamic Blocks)
    m_numThreads = (numThreads > 0) ? numThreads : std::thread::hardware_concurrency();
    if (m_numThreads == 0) m_numThreads = 8; // Default to 8 threads as specified in the PDF
}

MFPMethod3::~MFPMethod3() {
    // Clean up resources
}

bool MFPMethod3::isPrime(const std::string& number) {
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
    
    // Try to find a divisor using the Parallelized with Dynamic Blocks method
    bool hasDivisor = parallelizedFactorization(n, divisor);
    
    mpz_clears(n, divisor, nullptr);
    
    // If we found a divisor, the number is not prime
    return !hasDivisor;
}

std::vector<std::string> MFPMethod3::factorize(const std::string& number) {
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
    
    // Try to find a divisor using the Parallelized with Dynamic Blocks method
    if (parallelizedFactorization(n, divisor)) {
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

bool MFPMethod3::parallelizedFactorization(const mpz_t n, mpz_t divisor) {
    // Try each multiplier k in {1, 3, 7, 9}
    const int ks[] = {1, 3, 7, 9};
    
    for (int k : ks) {
        mpz_t nk, A, sqrtA;
        mpz_inits(nk, A, sqrtA, nullptr);
        
        // Calculate nk = n * k
        mpz_mul_ui(nk, n, k);
        
        // Calculate A = floor(nk / 10)
        mpz_fdiv_q_ui(A, nk, 10);
        
        // Calculate d0 = nk % 10
        unsigned long d0 = mpz_fdiv_ui(nk, 10);
        
        // Get A as unsigned long for calculations
        unsigned long A_ul = mpz_get_ui(A);
        
        // Calculate sqrt(A) for limits
        mpz_sqrt(sqrtA, A);
        unsigned long sqrtA_ul = mpz_get_ui(sqrtA);
        
        // Calculate i_max = sqrt(A)/10 + 2
        unsigned long i_max = sqrtA_ul / 10 + 2;
        
        // Calculate q_max = 2*sqrt(A)
        unsigned long q_max = sqrtA_ul * 2;
        
        // Calculate i_est (estimate for i)
        unsigned long i_est = sqrtA_ul / 10;
        
        // Atomic flag to indicate if a divisor was found
        std::atomic<bool> found_divisor(false);
        
        // Shared variable to store the found divisor
        unsigned long divisor_value = 0;
        
        // Mutex for thread synchronization
        std::mutex mtx;
        
        // Vector to hold thread objects
        std::vector<std::thread> threads;
        
        // Define the maximum number of blocks
        const int MAX_BLOCKS_I = 16;
        
        // Calculate block size for i search
        unsigned long block_size = (i_max + MAX_BLOCKS_I - 1) / MAX_BLOCKS_I;
        if (block_size < 1) block_size = 1;
        
        // Calculate block size for q sweep
        unsigned long q_block_size = (q_max + m_numThreads - 1) / m_numThreads;
        if (q_block_size < 1) q_block_size = 1;
        
        // Create threads for i search (alternating above and below i_est)
        for (int t = 0; t < m_numThreads / 2; ++t) {
            // Block above i_est
            unsigned long i_start_above = i_est + t * block_size;
            unsigned long i_end_above = i_start_above + block_size;
            if (i_end_above > i_max) i_end_above = i_max;
            
            if (i_start_above < i_end_above) {
                threads.push_back(std::thread(&MFPMethod3::searchBlock, this, 
                                             std::ref(n), k, i_start_above, i_end_above, 
                                             A_ul, d0, std::ref(found_divisor), 
                                             std::ref(divisor_value), std::ref(mtx)));
            }
            
            // Block below i_est
            if (i_est > t * block_size) {
                unsigned long i_start_below = i_est - (t + 1) * block_size;
                unsigned long i_end_below = i_est - t * block_size;
                
                if (i_start_below < i_end_below) {
                    threads.push_back(std::thread(&MFPMethod3::searchBlock, this, 
                                                 std::ref(n), k, i_start_below, i_end_below, 
                                                 A_ul, d0, std::ref(found_divisor), 
                                                 std::ref(divisor_value), std::ref(mtx)));
                }
            }
        }
        
        // Create threads for q sweep
        for (int t = 0; t < m_numThreads / 2; ++t) {
            unsigned long q_start = t * q_block_size + 1; // q starts from 1
            unsigned long q_end = q_start + q_block_size;
            if (q_end > q_max) q_end = q_max;
            
            if (q_start <= q_max) {
                threads.push_back(std::thread(&MFPMethod3::searchQSweep, this, 
                                             std::ref(n), k, q_start, q_end, 
                                             A_ul, d0, std::ref(found_divisor), 
                                             std::ref(divisor_value), std::ref(mtx)));
            }
        }
        
        // Wait for all threads to complete
        for (auto& thread : threads) {
            thread.join();
        }
        
        // Check if a divisor was found
        if (found_divisor) {
            mpz_set_ui(divisor, divisor_value);
            mpz_clears(nk, A, sqrtA, nullptr);
            return true;
        }
        
        mpz_clears(nk, A, sqrtA, nullptr);
    }
    
    return false;
}

void MFPMethod3::searchBlock(const mpz_t n, int k, unsigned long i_start, unsigned long i_end, 
                            unsigned long A_ul, unsigned long d0, std::atomic<bool>& found_divisor, 
                            unsigned long& divisor_value, std::mutex& mtx) {
    // Search for divisors in the given block of i values
    for (unsigned long i = i_start; i < i_end && !found_divisor; ++i) {
        // Calculate d = d0 + 10*i
        unsigned long d = d0 + 10 * i;
        
        // Skip if d <= 1
        if (d <= 1) continue;
        
        // Check if (A - i) % d == 0 (structural filter)
        unsigned long Ai = A_ul - i;
        if (Ai % d != 0) continue;
        
        // Check if n is divisible by d
        if (mpz_divisible_ui_p(n, d)) {
            // Found a divisor
            std::lock_guard<std::mutex> lock(mtx);
            if (!found_divisor) {
                found_divisor = true;
                divisor_value = d;
            }
            return;
        }
    }
}

void MFPMethod3::searchQSweep(const mpz_t n, int k, unsigned long q_start, unsigned long q_end, 
                             unsigned long A_ul, unsigned long d0, std::atomic<bool>& found_divisor, 
                             unsigned long& divisor_value, std::mutex& mtx) {
    // Search for divisors using q sweep in the given range
    for (unsigned long q = q_start; q <= q_end && !found_divisor; ++q) {
        // Calculate denom = 10*q + 1
        unsigned long denom = 10 * q + 1;
        
        // Calculate qd0 = q * d0
        unsigned long qd0 = q * d0;
        
        // Skip if qd0 > A
        if (qd0 > A_ul) continue;
        
        // Calculate numer = A - qd0
        unsigned long numer = A_ul - qd0;
        
        // Check if i is an integer (numer is divisible by denom)
        if (numer % denom != 0) continue;
        
        // Calculate i = numer / denom
        unsigned long i = numer / denom;
        
        // Calculate d = d0 + 10*i
        unsigned long d = d0 + 10 * i;
        
        // Skip if d <= 1
        if (d <= 1) continue;
        
        // Check if (A - i) % d == 0 (structural filter)
        unsigned long Ai = A_ul - i;
        if (Ai % d != 0) continue;
        
        // Check if n is divisible by d
        if (mpz_divisible_ui_p(n, d)) {
            // Found a divisor
            std::lock_guard<std::mutex> lock(mtx);
            if (!found_divisor) {
                found_divisor = true;
                divisor_value = d;
            }
            return;
        }
    }
}

} // namespace mfp
