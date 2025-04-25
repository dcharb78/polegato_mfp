#pragma once

#include "mfp_base.h"

namespace mfp {

class MFPMethod1 : public MFPBase {
public:
    MFPMethod1();
    virtual ~MFPMethod1();

    virtual bool isPrime(const std::string& number) override;
    virtual std::vector<std::string> factorize(const std::string& number) override;
    virtual std::string findNextPrime(const std::string& number) override;
    
private:
    // Method 1 specific implementation details (Expanded q Factorization)
    bool expandedQFactorization(const std::string& number, std::vector<std::string>& factors);
};

} // namespace mfp
