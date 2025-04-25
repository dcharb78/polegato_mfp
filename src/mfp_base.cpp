#include "mfp_base.h"
#include <gmp.h>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <random>

namespace mfp {

MFPBase::MFPBase() {
    // Initialize random seed for Miller-Rabin test
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
}

MFPBase::~MFPBase() {
    // Nothing to clean up
}

bool MFPBase::isPrime(const std::string& number) {
    // Try to convert to unsigned long for small numbers
    try {
        unsigned long n = std::stoul(number);
        if (n <= 1000000) {
            return isSmallPrime(n);
        }
    } catch (const std::exception& e) {
        // Number is too large for unsigned long, continue with GMP
    }
    
    // For larger numbers, use Miller-Rabin test
    return millerRabinTest(number);
}

std::vector<std::string> MFPBase::factorize(const std::string& number) {
    std::vector<std::string> factors;
    
    // Basic implementation that just returns the number itself if it's prime
    if (isPrime(number)) {
        factors.push_back(number);
        return factors;
    }
    
    // This is a placeholder - derived classes should implement better factorization
    factors.push_back(number);
    return factors;
}

std::string MFPBase::findNextPrime(const std::string& number) {
    mpz_t n, next_prime;
    mpz_init(n);
    mpz_init(next_prime);
    
    // Convert string to mpz_t
    mpz_set_str(n, number.c_str(), 10);
    
    // Find next prime
    mpz_nextprime(next_prime, n);
    
    // Convert back to string
    char* result_str = mpz_get_str(nullptr, 10, next_prime);
    std::string result(result_str);
    
    // Free resources
    free(result_str);
    mpz_clear(n);
    mpz_clear(next_prime);
    
    return result;
}

bool MFPBase::isSmallPrime(unsigned long n) {
    if (n <= 1) return false;
    if (n <= 3) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;
    
    for (unsigned long i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) return false;
    }
    
    return true;
}

bool MFPBase::millerRabinTest(const std::string& n, int iterations) {
    mpz_t num, a, r, y, j, minus_one;
    mpz_init(num);
    mpz_init(a);
    mpz_init(r);
    mpz_init(y);
    mpz_init(j);
    mpz_init(minus_one);
    
    // Convert string to mpz_t
    mpz_set_str(num, n.c_str(), 10);
    
    // Check if n is 2 or 3
    if (mpz_cmp_ui(num, 2) == 0 || mpz_cmp_ui(num, 3) == 0) {
        mpz_clear(num);
        mpz_clear(a);
        mpz_clear(r);
        mpz_clear(y);
        mpz_clear(j);
        mpz_clear(minus_one);
        return true;
    }
    
    // Check if n is even or less than 2
    if (mpz_even_p(num) != 0 || mpz_cmp_ui(num, 2) < 0) {
        mpz_clear(num);
        mpz_clear(a);
        mpz_clear(r);
        mpz_clear(y);
        mpz_clear(j);
        mpz_clear(minus_one);
        return false;
    }
    
    // Write n-1 as 2^s * d where d is odd
    mpz_sub_ui(r, num, 1);  // r = n-1
    unsigned int s = 0;
    while (mpz_even_p(r) != 0) {
        mpz_divexact_ui(r, r, 2);  // r = r/2
        s++;
    }
    
    // Set minus_one = n-1
    mpz_sub_ui(minus_one, num, 1);
    
    // Perform Miller-Rabin test
    std::random_device rd;
    std::mt19937 gen(rd());
    
    for (int i = 0; i < iterations; i++) {
        // Generate random a in [2, n-2]
        std::uniform_int_distribution<unsigned long> dist(2, mpz_get_ui(num) - 2);
        mpz_set_ui(a, dist(gen));
        
        // Compute y = a^r mod n
        mpz_powm(y, a, r, num);
        
        // If y != 1 and y != n-1
        if (mpz_cmp_ui(y, 1) != 0 && mpz_cmp(y, minus_one) != 0) {
            unsigned int j_val = 1;
            while (j_val < s && mpz_cmp(y, minus_one) != 0) {
                // y = y^2 mod n
                mpz_powm_ui(y, y, 2, num);
                
                // If y == 1, n is composite
                if (mpz_cmp_ui(y, 1) == 0) {
                    mpz_clear(num);
                    mpz_clear(a);
                    mpz_clear(r);
                    mpz_clear(y);
                    mpz_clear(j);
                    mpz_clear(minus_one);
                    return false;
                }
                
                j_val++;
            }
            
            // If y != n-1, n is composite
            if (mpz_cmp(y, minus_one) != 0) {
                mpz_clear(num);
                mpz_clear(a);
                mpz_clear(r);
                mpz_clear(y);
                mpz_clear(j);
                mpz_clear(minus_one);
                return false;
            }
        }
    }
    
    // Free resources
    mpz_clear(num);
    mpz_clear(a);
    mpz_clear(r);
    mpz_clear(y);
    mpz_clear(j);
    mpz_clear(minus_one);
    
    // If we get here, n is probably prime
    return true;
}

} // namespace mfp
