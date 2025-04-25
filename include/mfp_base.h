#pragma once

#include <string>
#include <vector>

namespace mfp {

class MFPBase {
public:
    MFPBase();
    virtual ~MFPBase();

    virtual bool isPrime(const std::string& number);
    virtual std::vector<std::string> factorize(const std::string& number);
    virtual std::string findNextPrime(const std::string& number);
    
protected:
    // Helper methods that can be used by derived classes
    bool isSmallPrime(unsigned long n);
    bool millerRabinTest(const std::string& n, int iterations = 40);
};

} // namespace mfp
