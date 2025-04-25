#pragma once

#include <gmp.h>
#include <vector>
#include <string>
#include <memory>

namespace mfp {

class MFPBase {
public:
    // Constructor and destructor
    MFPBase();
    virtual ~MFPBase();
    
    // Core methods (to be implemented by derived classes)
    virtual bool isPrime(const mpz_t& number) = 0;
    virtual bool findDivisor(const mpz_t& number, mpz_t& divisor_out) = 0;
    
    // Common utility methods
    bool factorize(const mpz_t& number, std::vector<mpz_t>& factors_out);
    
    // Performance logging
    void setPerformanceLogging(bool enable);
    struct PerformanceMetrics {
        double total_time;
        double isPrime_time;
        double findDivisor_time;
        size_t isPrime_calls;
        size_t findDivisor_calls;
        size_t total_digits_processed;
    };
    PerformanceMetrics getMetrics() const;
    
protected:
    // Common helper methods
    bool isSmallPrime(const mpz_t& number);
    bool hasSmallFactor(const mpz_t& number, mpz_t& factor_out);
    bool millerRabinTest(const mpz_t& number, int iterations);
    
    // Performance tracking
    bool performance_logging_enabled_;
    PerformanceMetrics metrics_;
};

// Factory function to create MFP implementation
std::unique_ptr<MFPBase> createMFPImplementation(int method_number, int num_threads);

} // namespace mfp
