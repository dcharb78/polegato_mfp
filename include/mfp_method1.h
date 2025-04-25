#pragma once

#include "mfp_base.h"

namespace mfp {

class MFPMethod1 : public MFPBase {
public:
    // Constructor and destructor
    MFPMethod1(int num_threads = 1);
    ~MFPMethod1() override;
    
    // Core methods implementation
    bool isPrime(const mpz_t& number) override;
    bool findDivisor(const mpz_t& number, mpz_t& divisor_out) override;
    
private:
    // Method-specific parameters
    int num_threads_;
    
    // Method-specific helper methods
    bool expandedQFactorization(const mpz_t& number, mpz_t& divisor_out);
    bool computeQExpansion(const mpz_t& number, std::vector<mpz_t>& q_values);
    bool findPatternInQSequence(const std::vector<mpz_t>& q_values, mpz_t& divisor_out);
};

} // namespace mfp
