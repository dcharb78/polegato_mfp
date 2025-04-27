#pragma once

#include "mfp_base.h"
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>

namespace mfp {

class MFPMethod3 : public MFPBase {
public:
    /**
     * Constructor for Method 3 (Parallelized with Dynamic Blocks)
     * @param numThreads Number of threads to use (0 for auto-detection)
     */
    MFPMethod3(int numThreads = 0);
    virtual ~MFPMethod3();

    /**
     * Determines if a number is prime using Method 3 (Parallelized with Dynamic Blocks)
     * @param number The number to check
     * @return true if the number is prime, false otherwise
     */
    virtual bool isPrime(const std::string& number) override;

    /**
     * Factorizes a number using Method 3 (Parallelized with Dynamic Blocks)
     * @param number The number to factorize
     * @return A vector of strings representing the factors
     */
    virtual std::vector<std::string> factorize(const std::string& number) override;

private:
    /**
     * Implements the Parallelized with Dynamic Blocks method
     * Uses the formula i = (A - qd0)/(10q + 1) to find potential divisors
     * with parallelized search in blocks centered on i_est
     * 
     * @param n The number to factorize
     * @param divisor Output parameter to store the found divisor
     * @return true if a divisor is found, false otherwise
     */
    bool parallelizedFactorization(const mpz_t n, mpz_t divisor);
    
    /**
     * Helper method to search for divisors in a specific block
     * @param n The number to factorize
     * @param k The multiplier to use (1, 3, 7, or 9)
     * @param i_start The starting i value for this block
     * @param i_end The ending i value for this block
     * @param A The A value (floor(nk / 10))
     * @param d0 The d0 value (nk % 10)
     * @param found_divisor Atomic flag to indicate if a divisor was found
     * @param divisor_value Shared variable to store the found divisor
     * @param mtx Mutex for thread synchronization
     * @param system_level The system level of the number being factorized
     */
    void searchBlock(const mpz_t n, int k, unsigned long i_start, unsigned long i_end, 
                    unsigned long A_ul, unsigned long d0, std::atomic<bool>& found_divisor, 
                    unsigned long& divisor_value, std::mutex& mtx, int system_level);
    
    /**
     * Helper method to search for divisors using q sweep
     * @param n The number to factorize
     * @param k The multiplier to use (1, 3, 7, or 9)
     * @param q_start The starting q value for this thread
     * @param q_end The ending q value for this thread
     * @param A The A value (floor(nk / 10))
     * @param d0 The d0 value (nk % 10)
     * @param found_divisor Atomic flag to indicate if a divisor was found
     * @param divisor_value Shared variable to store the found divisor
     * @param mtx Mutex for thread synchronization
     * @param system_level The system level of the number being factorized
     */
    void searchQSweep(const mpz_t n, int k, unsigned long q_start, unsigned long q_end, 
                     unsigned long A_ul, unsigned long d0, std::atomic<bool>& found_divisor, 
                     unsigned long& divisor_value, std::mutex& mtx, int system_level);
    
    /**
     * Determines the system level of a number
     * @param n The number to check
     * @return The system level (1-49)
     */
    int determineSystemLevel(const mpz_t n);
    
    /**
     * Filters the search space based on system level
     * @param system_level The system level of the number
     * @param i_max Original i_max value
     * @param q_max Original q_max value
     * @param i_est Original i_est value
     * @param filtered_i_max Output parameter for filtered i_max
     * @param filtered_q_max Output parameter for filtered q_max
     */
    void filterSearchSpace(int system_level, 
                          unsigned long i_max, unsigned long q_max, unsigned long i_est,
                          unsigned long& filtered_i_max, unsigned long& filtered_q_max);
    
    /**
     * Determines if a specific value should be included in the search
     * based on Fibonacci-related offsets and key positions
     * @param value The value to check
     * @param system_level The system level
     * @return true if the value should be included, false otherwise
     */
    bool shouldIncludeInSearch(unsigned long value, int system_level);
    
    int m_numThreads;  // Number of threads to use
};

} // namespace mfp
