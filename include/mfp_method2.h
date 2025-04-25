#pragma once

#include "mfp_base.h"

namespace mfp {

class MFPMethod2 : public MFPBase {
public:
    MFPMethod2();
    virtual ~MFPMethod2();

    virtual bool isPrime(const std::string& number) override;
    virtual std::vector<std::string> factorize(const std::string& number) override;
    virtual std::string findNextPrime(const std::string& number) override;
    
private:
    // Method 2 specific implementation details (Ultrafast with Structural Filter)
    bool structuralFilter(const std::string& number);
    bool ultrafastFactorization(const std::string& number, std::vector<std::string>& factors);
};

} // namespace mfp
