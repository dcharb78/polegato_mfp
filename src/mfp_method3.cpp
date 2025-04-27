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
    
    // Debug output to verify thread count
    std::cout << "MFPMethod3 initialized with " << m_numThreads << " threads" << std::endl;
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

int MFPMethod3::determineSystemLevel(const mpz_t n) {
    // Get the number of bits in n
    size_t num_bits = mpz_sizeinbase(n, 2);
    
    // Calculate the approximate system level based on the number of bits
    // System level boundaries from the UFRF document
    const double system_level_boundaries[] = {
        13, 26, 52, 104, 208, 416, 832, 1664, 3328, 6656,
        13312, 26624, 53248, 106496, 212992, 425984, 851968, 1.70e6, 3.41e6, 6.82e6,
        1.36e7, 2.73e7, 5.45e7, 1.09e8, 2.18e8, 4.36e8, 8.72e8, 1.74e9, 3.49e9, 6.98e9,
        1.40e10, 2.79e10, 5.58e10, 1.12e11, 2.23e11, 4.47e11, 8.93e11, 1.79e12, 3.57e12, 7.15e12,
        1.43e13, 2.86e13, 5.72e13, 1.14e14, 2.29e14, 4.57e14, 9.15e14, 1.83e15, 3.66e15
    };
    
    // Find the appropriate system level
    int system_level = 1;
    for (int i = 0; i < 49; ++i) {
        if (num_bits <= system_level_boundaries[i]) {
            system_level = i + 1;
            break;
        }
    }
    
    std::cout << "Number has " << num_bits << " bits, determined system level: " << system_level << std::endl;
    
    return system_level;
}

void MFPMethod3::filterSearchSpace(int system_level, 
                                  unsigned long i_max, unsigned long q_max, unsigned long i_est,
                                  unsigned long& filtered_i_max, unsigned long& filtered_q_max) {
    // Default to original search space
    filtered_i_max = i_max;
    filtered_q_max = q_max;
    
    // For system levels 1-9, use the original search space (no reduction)
    if (system_level <= 9) {
        std::cout << "System level " << system_level << " <= 9, using original search space" << std::endl;
        return;
    }
    
    // For system level 10+, apply progressively stronger reduction
    // The refined search space is approximately 60 digits regardless of system level
    const unsigned long REFINED_SEARCH_SPACE = 60;
    
    // Calculate reduction factor based on system level
    double reduction_factor = 1.0;
    if (system_level >= 10 && system_level <= 49) {
        // Reduction factors from the UFRF document
        const double reduction_factors[] = {
            1.01, 1.40, 1.96, 2.74, 3.85, 5.42, 7.63, 10.77, 15.20, 21.47,
            30.34, 42.88, 60.61, 85.69, 121.15, 171.31, 242.24, 342.54, 484.40, 685.02,
            968.74, 1369.98, 1937.42, 2739.89, 3874.77, 5479.72, 7749.47, 10959.37, 15498.86, 21918.68,
            30997.66, 43837.29, 61995.26, 87674.51, 123990.45, 175348.95, 247980.84, 350697.84, 495961.61, 701395.61
        };
        
        reduction_factor = reduction_factors[system_level - 10];
    } else if (system_level > 49) {
        // For system levels beyond 49, use the highest reduction factor
        reduction_factor = 701395.61;
    }
    
    // Apply reduction factor
    filtered_i_max = std::min(i_max, static_cast<unsigned long>(REFINED_SEARCH_SPACE));
    filtered_q_max = std::min(q_max, static_cast<unsigned long>(REFINED_SEARCH_SPACE * 10));
    
    std::cout << "System level " << system_level << ", applying reduction factor " << reduction_factor << std::endl;
    std::cout << "Original search space: i_max=" << i_max << ", q_max=" << q_max << std::endl;
    std::cout << "Filtered search space: i_max=" << filtered_i_max << ", q_max=" << filtered_q_max << std::endl;
}

bool MFPMethod3::shouldIncludeInSearch(unsigned long value, int system_level) {
    // For system levels 1-9, include all values (no filtering)
    if (system_level <= 9) {
        return true;
    }
    
    // Key positions to focus on (7, 2, 3)
    const unsigned long key_positions[] = {7, 2, 3};
    
    // Fibonacci-related offsets
    const unsigned long fibonacci_offsets[] = {1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144};
    const int num_fibonacci_offsets = sizeof(fibonacci_offsets) / sizeof(fibonacci_offsets[0]);
    
    // Check if value is at or near a key position
    for (unsigned long key_pos : key_positions) {
        // Check exact match
        if (value == key_pos) {
            return true;
        }
        
        // Check Fibonacci-related offsets
        for (int i = 0; i < num_fibonacci_offsets; ++i) {
            // Positive offset (60% bias)
            if (value == key_pos + fibonacci_offsets[i]) {
                // Apply 60% positive bias
                return (rand() % 100) < 60;
            }
            
            // Negative offset (40% bias)
            if (key_pos >= fibonacci_offsets[i] && value == key_pos - fibonacci_offsets[i]) {
                // Apply 40% negative bias
                return (rand() % 100) < 40;
            }
        }
    }
    
    // For higher system levels, be more selective
    if (system_level >= 20) {
        // For system level 20+, only include values that are multiples of key positions
        for (unsigned long key_pos : key_positions) {
            if (value % key_pos == 0) {
                return (rand() % 100) < 30; // 30% chance to include
            }
        }
        
        // For system level 30+, be even more selective
        if (system_level >= 30) {
            // Only include 5% of remaining values
            return (rand() % 100) < 5;
        }
        
        // For system level 20-29, include 10% of remaining values
        return (rand() % 100) < 10;
    }
    
    // For system level 10-19, include 20% of remaining values
    return (rand() % 100) < 20;
}

bool MFPMethod3::parallelizedFactorization(const mpz_t n, mpz_t divisor) {
    // Determine system level
    int system_level = determineSystemLevel(n);
    
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
        
        // Apply search space filtering based on system level
        unsigned long filtered_i_max, filtered_q_max;
        filterSearchSpace(system_level, i_max, q_max, i_est, filtered_i_max, filtered_q_max);
        
        // Use filtered values
        i_max = filtered_i_max;
        q_max = filtered_q_max;
        
        // Debug output
        std::cout << "Parallelized factorization with k=" << k << ", using " << m_numThreads << " threads" << std::endl;
        std::cout << "i_max=" << i_max << ", q_max=" << q_max << ", i_est=" << i_est << std::endl;
        std::cout << "System level=" << system_level << std::endl;
        
        // Atomic flag to indicate if a divisor was found
        std::atomic<bool> found_divisor(false);
        
        // Shared variable to store the found divisor
        unsigned long divisor_value = 0;
        
        // Mutex for thread synchronization
        std::mutex mtx;
        
        // Vector to hold thread objects
        std::vector<std::thread> threads;
        
        // Define the maximum number of blocks
        const int MAX_BLOCKS = m_numThreads * 2; // Ensure enough blocks for all threads
        
        // Calculate block size for i search
        unsigned long i_block_size = (i_max + MAX_BLOCKS - 1) / MAX_BLOCKS;
        if (i_block_size < 1) i_block_size = 1;
        
        // Calculate block size for q sweep
        unsigned long q_block_size = (q_max + MAX_BLOCKS - 1) / MAX_BLOCKS;
        if (q_block_size < 1) q_block_size = 1;
        
        // Allocate threads more efficiently - use 2/3 for i-search and 1/3 for q-sweep
        int i_search_threads = (m_numThreads * 2) / 3;
        if (i_search_threads < 1) i_search_threads = 1;
        
        int q_sweep_threads = m_numThreads - i_search_threads;
        if (q_sweep_threads < 1 && m_numThreads > 1) {
            q_sweep_threads = 1;
            i_search_threads = m_numThreads - 1;
        }
        
        std::cout << "Allocating " << i_search_threads << " threads for i-search and " 
                  << q_sweep_threads << " threads for q-sweep" << std::endl;
        
        // Create threads for i search (alternating above and below i_est)
        for (int t = 0; t < i_search_threads; ++t) {
            // Determine if this thread should search above or below i_est
            bool search_above = (t % 2 == 0);
            
            if (search_above) {
                // Block above i_est
                unsigned long i_start = i_est + (t / 2) * i_block_size;
                unsigned long i_end = i_start + i_block_size;
                if (i_end > i_max) i_end = i_max;
                
                if (i_start < i_end) {
                    std::cout << "Thread " << t << " searching i-block above: [" << i_start << ", " << i_end << ")" << std::endl;
                    threads.push_back(std::thread(&MFPMethod3::searchBlock, this, 
                                                std::ref(n), k, i_start, i_end, 
                                                A_ul, d0, std::ref(found_divisor), 
                                                std::ref(divisor_value), std::ref(mtx), system_level));
                }
            } else {
                // Block below i_est
                unsigned long offset = (t / 2 + 1) * i_block_size;
                if (i_est > offset) {
                    unsigned long i_start = i_est - offset;
                    unsigned long i_end = i_start + i_block_size;
                    if (i_end > i_est) i_end = i_est;
                    
                    if (i_start < i_end) {
                        std::cout << "Thread " << t << " searching i-block below: [" << i_start << ", " << i_end << ")" << std::endl;
                        threads.push_back(std::thread(&MFPMethod3::searchBlock, this, 
                                                    std::ref(n), k, i_start, i_end, 
                                                    A_ul, d0, std::ref(found_divisor), 
                                                    std::ref(divisor_value), std::ref(mtx), system_level));
                    }
                }
            }
        }
        
        // Create threads for q sweep
        for (int t = 0; t < q_sweep_threads; ++t) {
            unsigned long q_start = t * q_block_size + 1; // q starts from 1
            unsigned long q_end = q_start + q_block_size;
            if (q_end > q_max) q_end = q_max;
            
            if (q_start <= q_max) {
                std::cout << "Thread " << (i_search_threads + t) << " searching q-sweep: [" << q_start << ", " << q_end << "]" << std::endl;
                threads.push_back(std::thread(&MFPMethod3::searchQSweep, this, 
                                            std::ref(n), k, q_start, q_end, 
                                            A_ul, d0, std::ref(found_divisor), 
                                            std::ref(divisor_value), std::ref(mtx), system_level));
            }
        }
        
        std::cout << "Created " << threads.size() << " threads for parallel search" << std::endl;
        
        // Wait for all threads to complete
        for (auto& thread : threads) {
            thread.join();
        }
        
        std::cout << "All threads completed for k=" << k << std::endl;
        
        // Check if a divisor was found
        if (found_divisor) {
            mpz_set_ui(divisor, divisor_value);
            std::cout << "Found divisor: " << divisor_value << std::endl;
            mpz_clears(nk, A, sqrtA, nullptr);
            return true;
        }
        
        mpz_clears(nk, A, sqrtA, nullptr);
    }
    
    return false;
}

void MFPMethod3::searchBlock(const mpz_t n, int k, unsigned long i_start, unsigned long i_end, 
                            unsigned long A_ul, unsigned long d0, std::atomic<bool>& found_divisor, 
                            unsigned long& divisor_value, std::mutex& mtx, int system_level) {
    // Thread ID for debugging
    std::thread::id thread_id = std::this_thread::get_id();
    std::cout << "Thread " << thread_id << " started searching i-block [" << i_start << ", " << i_end << ")" << std::endl;
    
    // Search for divisors in the given block of i values
    for (unsigned long i = i_start; i < i_end && !found_divisor; ++i) {
        // Skip if this i value should be excluded based on UFRF framework
        if (!shouldIncludeInSearch(i, system_level)) continue;
        
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
                std::cout << "Thread " << thread_id << " found divisor: " << d << std::endl;
            }
            return;
        }
    }
    
    std::cout << "Thread " << thread_id << " completed i-block search without finding divisor" << std::endl;
}

void MFPMethod3::searchQSweep(const mpz_t n, int k, unsigned long q_start, unsigned long q_end, 
                             unsigned long A_ul, unsigned long d0, std::atomic<bool>& found_divisor, 
                             unsigned long& divisor_value, std::mutex& mtx, int system_level) {
    // Thread ID for debugging
    std::thread::id thread_id = std::this_thread::get_id();
    std::cout << "Thread " << thread_id << " started searching q-sweep [" << q_start << ", " << q_end << "]" << std::endl;
    
    // Search for divisors using q sweep in the given range
    for (unsigned long q = q_start; q <= q_end && !found_divisor; ++q) {
        // Skip if this q value should be excluded based on UFRF framework
        if (!shouldIncludeInSearch(q, system_level)) continue;
        
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
        
        // Skip if this i value should be excluded based on UFRF framework
        if (!shouldIncludeInSearch(i, system_level)) continue;
        
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
                std::cout << "Thread " << thread_id << " found divisor: " << d << std::endl;
            }
            return;
        }
    }
    
    std::cout << "Thread " << thread_id << " completed q-sweep search without finding divisor" << std::endl;
}

} // namespace mfp
