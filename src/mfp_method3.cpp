#include "mfp_method3.h"
#include <gmp.h>
#include <iostream>
#include <cmath>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>

namespace mfp {

MFPMethod3::MFPMethod3(int numThreads) {
    // Initialize Method 3 (Parallelized with Dynamic Blocks)
    m_numThreads = (numThreads > 0) ? numThreads : std::thread::hardware_concurrency();
    if (m_numThreads == 0) m_numThreads = 1; // Fallback to single thread
}

MFPMethod3::~MFPMethod3() {
    // Nothing to clean up
}

bool MFPMethod3::isPrime(const std::string& number) {
    // For small numbers, use the base class implementation
    try {
        unsigned long n = std::stoul(number);
        if (n < 1000000) {
            return MFPBase::isPrime(number);
        }
    } catch (const std::exception& e) {
        // Number is too large for unsigned long, continue with parallel implementation
    }
    
    // For larger numbers, use parallel primality test
    return parallelPrimalityTest(number);
}

std::vector<std::string> MFPMethod3::factorize(const std::string& number) {
    std::vector<std::string> factors;
    
    // If the number is prime, just return it
    if (isPrime(number)) {
        factors.push_back(number);
        return factors;
    }
    
    // Use the parallel factorization method
    if (parallelFactorization(number, factors)) {
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

std::string MFPMethod3::findNextPrime(const std::string& number) {
    // Use the base class implementation
    return MFPBase::findNextPrime(number);
}

bool MFPMethod3::parallelPrimalityTest(const std::string& number) {
    // Convert string to mpz_t
    mpz_t n;
    mpz_init(n);
    mpz_set_str(n, number.c_str(), 10);
    
    // Check if n is 2 or 3
    if (mpz_cmp_ui(n, 2) == 0 || mpz_cmp_ui(n, 3) == 0) {
        mpz_clear(n);
        return true;
    }
    
    // Check if n is even or less than 2
    if (mpz_even_p(n) != 0 || mpz_cmp_ui(n, 2) < 0) {
        mpz_clear(n);
        return false;
    }
    
    // Write n-1 as 2^s * d where d is odd
    mpz_t d;
    mpz_init(d);
    mpz_sub_ui(d, n, 1);  // d = n-1
    unsigned int s = 0;
    while (mpz_even_p(d) != 0) {
        mpz_divexact_ui(d, d, 2);  // d = d/2
        s++;
    }
    
    // Number of Miller-Rabin iterations
    const int iterations = 40;
    
    // Atomic flag to indicate if a witness was found
    std::atomic<bool> is_composite(false);
    
    // Mutex for thread synchronization
    std::mutex mtx;
    
    // Vector to hold thread objects
    std::vector<std::thread> threads;
    
    // Function to test a range of witnesses
    auto test_witnesses = [&](int start, int end) {
        // Create thread-local GMP variables
        mpz_t a, y, j, n_local, d_local, n_minus_1;
        mpz_init(a);
        mpz_init(y);
        mpz_init(j);
        mpz_init(n_local);
        mpz_init(d_local);
        mpz_init(n_minus_1);
        
        // Copy shared variables to thread-local variables
        {
            std::lock_guard<std::mutex> lock(mtx);
            mpz_set(n_local, n);
            mpz_set(d_local, d);
            mpz_sub_ui(n_minus_1, n_local, 1);
        }
        
        // Test witnesses in the assigned range
        for (int i = start; i < end && !is_composite; i++) {
            // Set a to i+2 (witnesses start from 2)
            mpz_set_ui(a, i + 2);
            
            // Compute y = a^d mod n
            mpz_powm(y, a, d_local, n_local);
            
            // If y == 1 or y == n-1, continue with next witness
            if (mpz_cmp_ui(y, 1) == 0 || mpz_cmp(y, n_minus_1) == 0) {
                continue;
            }
            
            // Perform the remaining r-1 squarings
            bool may_be_prime = false;
            for (unsigned int r = 1; r < s; r++) {
                // y = y^2 mod n
                mpz_powm_ui(y, y, 2, n_local);
                
                // If y == n-1, break and continue with next witness
                if (mpz_cmp(y, n_minus_1) == 0) {
                    may_be_prime = true;
                    break;
                }
                
                // If y == 1, n is composite
                if (mpz_cmp_ui(y, 1) == 0) {
                    is_composite = true;
                    break;
                }
            }
            
            // If we didn't find a reason to believe n might be prime, it's composite
            if (!may_be_prime && !is_composite) {
                is_composite = true;
            }
        }
        
        // Free thread-local GMP variables
        mpz_clear(a);
        mpz_clear(y);
        mpz_clear(j);
        mpz_clear(n_local);
        mpz_clear(d_local);
        mpz_clear(n_minus_1);
    };
    
    // Divide the work among threads
    int witnesses_per_thread = iterations / m_numThreads;
    int remaining_witnesses = iterations % m_numThreads;
    
    int start = 0;
    for (int i = 0; i < m_numThreads; i++) {
        int thread_witnesses = witnesses_per_thread + (i < remaining_witnesses ? 1 : 0);
        int end = start + thread_witnesses;
        
        threads.push_back(std::thread(test_witnesses, start, end));
        start = end;
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Free GMP variables
    mpz_clear(n);
    mpz_clear(d);
    
    // Return result
    return !is_composite;
}

bool MFPMethod3::parallelFactorization(const std::string& number, std::vector<std::string>& factors) {
    mpz_t n;
    mpz_init(n);
    
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
        return true;
    }
    
    // Mutex for thread synchronization
    std::mutex mtx;
    
    // Atomic flag to indicate if a factor was found
    std::atomic<bool> factor_found(false);
    
    // Vector to hold thread objects
    std::vector<std::thread> threads;
    
    // Shared variables for the found factor
    mpz_t found_factor;
    mpz_init(found_factor);
    
    // Function to search for factors in a range
    auto search_factors = [&](int thread_id) {
        // Create thread-local GMP variables
        mpz_t x, y, d, n_local, c;
        mpz_init(x);
        mpz_init(y);
        mpz_init(d);
        mpz_init(n_local);
        mpz_init(c);
        
        // Copy shared variables to thread-local variables
        {
            std::lock_guard<std::mutex> lock(mtx);
            mpz_set(n_local, n);
        }
        
        // Set c to a different value for each thread to increase chances of finding factors
        mpz_set_ui(c, thread_id + 1);
        
        // Initialize x and y to 2
        mpz_set_ui(x, 2);
        mpz_set_ui(y, 2);
        
        // Initialize d to 1
        mpz_set_ui(d, 1);
        
        // Define the polynomial f(x) = x^2 + c mod n
        auto f = [&](mpz_t result, const mpz_t x) {
            mpz_mul(result, x, x);
            mpz_add(result, result, c);
            mpz_mod(result, result, n_local);
        };
        
        // Main loop
        int iterations = 0;
        while (mpz_cmp_ui(d, 1) == 0 && !factor_found && iterations < 100000) {
            // x = f(x)
            f(x, x);
            
            // y = f(f(y))
            f(y, y);
            f(y, y);
            
            // d = gcd(|x - y|, n)
            mpz_sub(d, x, y);
            mpz_abs(d, d);
            mpz_gcd(d, d, n_local);
            
            iterations++;
        }
        
        // Check if we found a proper factor
        if (mpz_cmp_ui(d, 1) > 0 && mpz_cmp(d, n_local) < 0) {
            // Found a factor
            std::lock_guard<std::mutex> lock(mtx);
            if (!factor_found) {
                factor_found = true;
                mpz_set(found_factor, d);
            }
        }
        
        // Free thread-local GMP variables
        mpz_clear(x);
        mpz_clear(y);
        mpz_clear(d);
        mpz_clear(n_local);
        mpz_clear(c);
    };
    
    // Create threads
    for (int i = 0; i < m_numThreads; i++) {
        threads.push_back(std::thread(search_factors, i));
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Check if a factor was found
    if (factor_found) {
        // Add the found factor to the list
        char* factor1 = mpz_get_str(nullptr, 10, found_factor);
        factors.push_back(std::string(factor1));
        free(factor1);
        
        // Calculate the other factor
        mpz_t other_factor;
        mpz_init(other_factor);
        mpz_divexact(other_factor, n, found_factor);
        
        // Add the other factor to the list
        char* factor2 = mpz_get_str(nullptr, 10, other_factor);
        factors.push_back(std::string(factor2));
        free(factor2);
        
        // Free GMP variables
        mpz_clear(other_factor);
        mpz_clear(found_factor);
        mpz_clear(n);
        
        return true;
    }
    
    // If we get here, the method failed to find factors
    mpz_clear(found_factor);
    mpz_clear(n);
    
    return false;
}

} // namespace mfp
