#include "mfp_system.h"
#include <iostream>
#include <thread>

namespace mfp {

MFPSystem::MFPSystem(MFPMethodType method, int numThreads) 
    : m_methodType(method), m_numThreads(numThreads) {
    // If numThreads is not specified, use all available cores
    if (m_numThreads <= 0) {
        m_numThreads = std::thread::hardware_concurrency();
        if (m_numThreads == 0) m_numThreads = 1; // Fallback to single thread
    }
    
    // Create the appropriate method
    createMethod();
}

MFPSystem::~MFPSystem() {
    // m_method is a unique_ptr, so it will be automatically deleted
}

bool MFPSystem::isPrime(const std::string& number) {
    if (!m_method) {
        createMethod();
    }
    
    return m_method->isPrime(number);
}

std::vector<std::string> MFPSystem::factorize(const std::string& number) {
    if (!m_method) {
        createMethod();
    }
    
    return m_method->factorize(number);
}

std::string MFPSystem::findNextPrime(const std::string& number) {
    if (!m_method) {
        createMethod();
    }
    
    return m_method->findNextPrime(number);
}

void MFPSystem::setMethod(MFPMethodType method) {
    if (m_methodType != method) {
        m_methodType = method;
        createMethod();
    }
}

MFPMethodType MFPSystem::getMethod() const {
    return m_methodType;
}

void MFPSystem::createMethod() {
    // Create the appropriate method based on the method type
    switch (m_methodType) {
        case MFPMethodType::METHOD_1:
            m_method = std::make_unique<MFPMethod1>();
            break;
        case MFPMethodType::METHOD_2:
            m_method = std::make_unique<MFPMethod2>();
            break;
        case MFPMethodType::METHOD_3:
            m_method = std::make_unique<MFPMethod3>(m_numThreads);
            break;
        case MFPMethodType::AUTO:
            // For AUTO, use Method 3 if we have multiple cores, otherwise use Method 2
            if (m_numThreads > 1) {
                m_method = std::make_unique<MFPMethod3>(m_numThreads);
            } else {
                m_method = std::make_unique<MFPMethod2>();
            }
            break;
    }
}

} // namespace mfp
