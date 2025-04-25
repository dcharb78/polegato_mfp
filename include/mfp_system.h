#pragma once

#include "mfp_base.h"
#include "mfp_method1.h"
#include "mfp_method2.h"
#include "mfp_method3.h"
#include <memory>

namespace mfp {

enum class MFPMethodType {
    METHOD_1, // Expanded q Factorization
    METHOD_2, // Ultrafast with Structural Filter
    METHOD_3, // Parallelized with Dynamic Blocks
    AUTO      // Automatically select the best method
};

class MFPSystem {
public:
    MFPSystem(MFPMethodType method = MFPMethodType::AUTO, int numThreads = 0);
    ~MFPSystem();

    bool isPrime(const std::string& number);
    std::vector<std::string> factorize(const std::string& number);
    std::string findNextPrime(const std::string& number);
    
    void setMethod(MFPMethodType method);
    MFPMethodType getMethod() const;
    
private:
    MFPMethodType m_methodType;
    std::unique_ptr<MFPBase> m_method;
    int m_numThreads;
    
    void createMethod();
};

} // namespace mfp
