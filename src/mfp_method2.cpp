#include "mfp_method2.h"
#include <gmp.h>
#include <iostream>
#include <cmath>
#include <vector>

namespace mfp {

MFPMethod2::MFPMethod2() {
    // Initialize Method 2 (Ultrafast with Structural Filter)
}

MFPMethod2::~MFPMethod2() {
    // Nothing to clean up
}

bool MFPMethod2::isPrime(const std::string& number) {
    // First apply structural filter for quick rejection
    if (!structuralFilter(number)) {
        return false;
    }
    
    // Then use Miller-Rabin test for more thorough check
    return MFPBase::isPrime(number);
}

std::vector<std::string> MFPMethod2::factorize(const std::string& number) {
    std::vector<std::string> factors;
    
    // If the number is prime, just return it
    if (isPrime(number)) {
        factors.push_back(number);
        return factors;
    }
    
    // Use the Ultrafast Factorization method
    if (ultrafastFactorization(number, factors)) {
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

std::string MFPMethod2::findNextPrime(const std::string& number) {
    // Use the base class implementation
    return MFPBase::findNextPrime(number);
}

bool MFPMethod2::structuralFilter(const std::string& number) {
    // Quick check for small numbers
    try {
        unsigned long n = std::stoul(number);
        if (n <= 1) return false;
        if (n <= 3) return true;
        if (n % 2 == 0 || n % 3 == 0) return false;
        
        // Check if n is divisible by small primes
        const unsigned long small_primes[] = {5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47};
        for (unsigned long p : small_primes) {
            if (n % p == 0) return n == p;
        }
        
        return true;
    } catch (const std::exception& e) {
        // Number is too large for unsigned long, continue with GMP
    }
    
    mpz_t n;
    mpz_init(n);
    
    // Convert string to mpz_t
    mpz_set_str(n, number.c_str(), 10);
    
    // Check if n is divisible by small primes
    const unsigned long small_primes[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47};
    for (unsigned long p : small_primes) {
        if (mpz_divisible_ui_p(n, p) != 0) {
            // Check if n is equal to p
            if (mpz_cmp_ui(n, p) == 0) {
                mpz_clear(n);
                return true;
            }
            
            mpz_clear(n);
            return false;
        }
    }
    
    // Check if n passes Fermat's little theorem for base 2
    // If 2^(n-1) ≡ 1 (mod n), n might be prime
    mpz_t base, exp, result;
    mpz_init_set_ui(base, 2);
    mpz_init(exp);
    mpz_init(result);
    
    // exp = n - 1
    mpz_sub_ui(exp, n, 1);
    
    // result = 2^(n-1) mod n
    mpz_powm(result, base, exp, n);
    
    // Check if result is 1
    bool is_probable_prime = (mpz_cmp_ui(result, 1) == 0);
    
    // Free resources
    mpz_clear(n);
    mpz_clear(base);
    mpz_clear(exp);
    mpz_clear(result);
    
    return is_probable_prime;
}

bool MFPMethod2::ultrafastFactorization(const std::string& number, std::vector<std::string>& factors) {
    mpz_t n, factor, temp;
    mpz_init(n);
    mpz_init(factor);
    mpz_init(temp);
    
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
        mpz_clear(factor);
        mpz_clear(temp);
        
        return true;
    }
    
    // Try Pollard's rho algorithm
    mpz_t x, y, d, one;
    mpz_init(x);
    mpz_init(y);
    mpz_init(d);
    mpz_init_set_ui(one, 1);
    
    // Initialize x and y to 2
    mpz_set_ui(x, 2);
    mpz_set_ui(y, 2);
    
    // Initialize d to 1
    mpz_set_ui(d, 1);
    
    // Define the polynomial f(x) = x^2 + 1 mod n
    auto f = [&](mpz_t result, const mpz_t x) {
        mpz_mul(result, x, x);
        mpz_add_ui(result, result, 1);
        mpz_mod(result, result, n);
    };
    
    // Main loop
    while (mpz_cmp_ui(d, 1) == 0) {
        // x = f(x)
        f(x, x);
        
        // y = f(f(y))
        f(y, y);
        f(y, y);
        
        // d = gcd(|x - y|, n)
        mpz_sub(temp, x, y);
        mpz_abs(temp, temp);
        mpz_gcd(d, temp, n);
    }
    
    // Check if we found a proper factor
    if (mpz_cmp(d, n) != 0) {
        // Found a factor
        char* factor1 = mpz_get_str(nullptr, 10, d);
        factors.push_back(std::string(factor1));
        free(factor1);
        
        // Calculate the other factor
        mpz_divexact(temp, n, d);
        char* factor2 = mpz_get_str(nullptr, 10, temp);
        factors.push_back(std::string(factor2));
        free(factor2);
        
        mpz_clear(n);
        mpz_clear(factor);
        mpz_clear(temp);
        mpz_clear(x);
        mpz_clear(y);
        mpz_clear(d);
        mpz_clear(one);
        
        return true;
    }
    
    // If we get here, the method failed to find factors
    mpz_clear(n);
    mpz_clear(factor);
    mpz_clear(temp);
    mpz_clear(x);
    mpz_clear(y);
    mpz_clear(d);
    mpz_clear(one);
    
    return false;
}

} // namespace mfp
