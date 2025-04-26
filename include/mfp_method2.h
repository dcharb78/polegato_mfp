#pragma once

#include "mfp_base.h"

namespace mfp {

class MFPMethod2 : public MFPBase {
public:
    MFPMethod2();
    virtual ~MFPMethod2();

    /**
     * Determines if a number is prime using Method 2 (Ultrafast with Structural Filter)
     * @param number The number to check
     * @return true if the number is prime, false otherwise
     */
    virtual bool isPrime(const std::string& number) override;

    /**
     * Factorizes a number using Method 2 (Ultrafast with Structural Filter)
     * @param number The number to factorize
     * @return A vector of strings representing the factors
     */
    virtual std::vector<std::string> factorize(const std::string& number) override;

private:
    /**
     * Implements the Ultrafast with Structural Filter method
     * Uses the formula i = (A - qd0)/(10q + 1) to find potential divisors
     * with q_max = 2*sqrt(A) and additional structural filter (A - i) % d == 0
     * 
     * @param n The number to factorize
     * @param divisor Output parameter to store the found divisor
     * @return true if a divisor is found, false otherwise
     */
    bool ultrafastFactorization(const mpz_t n, mpz_t divisor);
    
    /**
     * Helper method to test a specific multiplier k
     * @param n The number to factorize
     * @param k The multiplier to use (1, 3, 7, or 9)
     * @param divisor Output parameter to store the found divisor
     * @return true if a divisor is found, false otherwise
     */
    bool testUltrafastDivisor(const mpz_t n, int k, mpz_t divisor);
};

} // namespace mfp
