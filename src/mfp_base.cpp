#include "../include/mfp_base.h"
#include <iostream>
#include <cmath>

namespace mfp {

MFPBase::MFPBase() {
    // Initialize base class
}

MFPBase::~MFPBase() {
    // Clean up resources
}

bool MFPBase::isPrime(const std::string& number) {
    // Check if the number is small enough to use trial division
    try {
        unsigned long n = std::stoul(number);
        if (n <= 1) return false;
        if (n <= 3) return true;
        if (n % 2 == 0 || n % 3 == 0) return false;
        
        // Trial division for small numbers
        for (unsigned long i = 5; i * i <= n; i += 6) {
            if (n % i == 0 || n % (i + 2) == 0) return false;
        }
        
        return true;
    } catch (const std::exception& e) {
        // Number is too large for unsigned long, use Miller-Rabin test
    }
    
    // For larger numbers, use Miller-Rabin primality test
    return millerRabinTest(number);
}

std::string MFPBase::findNextPrime(const std::string& number) {
    mpz_t n, next;
    mpz_inits(n, next, nullptr);
    
    // Convert string to mpz_t
    strToMpz(number, n);
    
    // Add 1 to get the next number
    mpz_add_ui(next, n, 1);
    
    // If the number is even, make it odd
    if (mpz_even_p(next)) {
        mpz_add_ui(next, next, 1);
    }
    
    // Keep incrementing by 2 until we find a prime
    while (!isPrime(mpzToStr(next))) {
        mpz_add_ui(next, next, 2);
    }
    
    // Convert result to string
    std::string result = mpzToStr(next);
    
    // Clean up
    mpz_clears(n, next, nullptr);
    
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

bool MFPBase::millerRabinTest(const std::string& number, int iterations) {
    mpz_t n, a, d, x, y, n_minus_1;
    mpz_inits(n, a, d, x, y, n_minus_1, nullptr);
    
    // Convert string to mpz_t
    strToMpz(number, n);
    
    // Check if n is 2 or 3
    if (mpz_cmp_ui(n, 2) == 0 || mpz_cmp_ui(n, 3) == 0) {
        mpz_clears(n, a, d, x, y, n_minus_1, nullptr);
        return true;
    }
    
    // Check if n is even or less than 2
    if (mpz_even_p(n) != 0 || mpz_cmp_ui(n, 2) < 0) {
        mpz_clears(n, a, d, x, y, n_minus_1, nullptr);
        return false;
    }
    
    // Write n-1 as 2^s * d where d is odd
    mpz_sub_ui(n_minus_1, n, 1);  // n_minus_1 = n-1
    mpz_set(d, n_minus_1);        // d = n-1
    unsigned int s = 0;
    while (mpz_even_p(d) != 0) {
        mpz_divexact_ui(d, d, 2);  // d = d/2
        s++;
    }
    
    // Witness loop
    gmp_randstate_t rstate;
    gmp_randinit_default(rstate);
    
    for (int i = 0; i < iterations; i++) {
        // Generate a random base a in [2, n-2]
        mpz_sub_ui(n_minus_1, n, 3);  // n_minus_1 = n-3
        mpz_urandomm(a, rstate, n_minus_1);  // a = random in [0, n-4)
        mpz_add_ui(a, a, 2);  // a = random in [2, n-2]
        
        // Compute x = a^d mod n
        mpz_powm(x, a, d, n);
        
        // If x == 1 or x == n-1, continue with next witness
        if (mpz_cmp_ui(x, 1) == 0 || mpz_cmp(x, n_minus_1) == 0) {
            continue;
        }
        
        // Perform s-1 squarings
        bool may_be_prime = false;
        for (unsigned int r = 1; r < s; r++) {
            // x = x^2 mod n
            mpz_powm_ui(x, x, 2, n);
            
            // If x == n-1, break and continue with next witness
            if (mpz_cmp(x, n_minus_1) == 0) {
                may_be_prime = true;
                break;
            }
            
            // If x == 1, n is composite
            if (mpz_cmp_ui(x, 1) == 0) {
                gmp_randclear(rstate);
                mpz_clears(n, a, d, x, y, n_minus_1, nullptr);
                return false;
            }
        }
        
        // If we didn't find a reason to believe n might be prime, it's composite
        if (!may_be_prime) {
            gmp_randclear(rstate);
            mpz_clears(n, a, d, x, y, n_minus_1, nullptr);
            return false;
        }
    }
    
    // If we get here, n is probably prime
    gmp_randclear(rstate);
    mpz_clears(n, a, d, x, y, n_minus_1, nullptr);
    return true;
}

void MFPBase::strToMpz(const std::string& s, mpz_t out) {
    mpz_set_str(out, s.c_str(), 10);
}

std::string MFPBase::mpzToStr(const mpz_t x) {
    char* s = mpz_get_str(nullptr, 10, x);
    std::string result(s);
    free(s);
    return result;
}

bool MFPBase::checkSmallPrimes(const mpz_t n, mpz_t divisor) {
    // Check if n is divisible by small primes (2, 3, 5)
    if (mpz_divisible_ui_p(n, 2) != 0) {
        mpz_set_ui(divisor, 2);
        return true;
    }
    
    if (mpz_divisible_ui_p(n, 3) != 0) {
        mpz_set_ui(divisor, 3);
        return true;
    }
    
    if (mpz_divisible_ui_p(n, 5) != 0) {
        mpz_set_ui(divisor, 5);
        return true;
    }
    
    return false;
}

bool MFPBase::isEven(const mpz_t n) {
    return mpz_even_p(n) != 0;
}

} // namespace mfp
