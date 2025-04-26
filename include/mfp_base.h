#pragma once

#include <string>
#include <vector>
#include <gmp.h>

namespace mfp {

class MFPBase {
public:
    MFPBase();
    virtual ~MFPBase();

    /**
     * Determines if a number is prime
     * @param number The number to check
     * @return true if the number is prime, false otherwise
     */
    virtual bool isPrime(const std::string& number);

    /**
     * Factorizes a number into its prime factors
     * @param number The number to factorize
     * @return A vector of strings representing the prime factors
     */
    virtual std::vector<std::string> factorize(const std::string& number) = 0;

    /**
     * Finds the next prime number after the given number
     * @param number The starting number
     * @return The next prime number
     */
    virtual std::string findNextPrime(const std::string& number);

protected:
    // Helper methods for common operations
    bool isSmallPrime(unsigned long n);
    bool millerRabinTest(const std::string& n, int iterations = 40);
    
    // Helper methods for GMP operations
    void strToMpz(const std::string& s, mpz_t out);
    std::string mpzToStr(const mpz_t x);
    
    // Common MFP operations
    bool checkSmallPrimes(const mpz_t n, mpz_t divisor);
    bool isEven(const mpz_t n);
};

} // namespace mfp
