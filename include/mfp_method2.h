#pragma once

#include "mfp_base.h"

namespace mfp {

class MFPMethod2 : public MFPBase {
public:
    // Constructor and destructor
    MFPMethod2(int num_threads = 1);
    ~MFPMethod2() override;
    
    // Core methods implementation
    bool isPrime(const mpz_t& number) override;
    bool findDivisor(const mpz_t& number, mpz_t& divisor_out) override;
    
private:
    // Method-specific parameters
    int num_threads_;
    
    // Method-specific helper methods
    bool applyStructuralFilter(const mpz_t& number);
    bool ultraFastFactorization(const mpz_t& number, mpz_t& divisor_out);
    bool verifyPrimality(const mpz_t& number);
};

} // namespace mfp
