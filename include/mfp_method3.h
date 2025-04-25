#pragma once

#include "mfp_base.h"
#include <thread>
#include <mutex>
#include <vector>

namespace mfp {

class MFPMethod3 : public MFPBase {
public:
    MFPMethod3(int numThreads = 0);
    virtual ~MFPMethod3();

    virtual bool isPrime(const std::string& number) override;
    virtual std::vector<std::string> factorize(const std::string& number) override;
    virtual std::string findNextPrime(const std::string& number) override;
    
private:
    // Method 3 specific implementation details (Parallelized with Dynamic Blocks)
    bool parallelPrimalityTest(const std::string& number);
    bool parallelFactorization(const std::string& number, std::vector<std::string>& factors);
    
    int m_numThreads;
    std::vector<std::thread> m_threads;
    std::mutex m_mutex;
};

} // namespace mfp
