#pragma once

#include "mfp_base.h"
#include "prime_database.h"
#include "performance_monitor.h"
#include "parallel_framework.h"
#include <memory>
#include <string>
#include <gmp.h>

namespace mfp {

class MFPSystem {
public:
    // Configuration options
    struct Config {
        int method = 3;                      // MFP method (1, 2, or 3)
        int num_threads = 32;                // Number of threads to use
        std::string db_path = "./primes_db"; // Path to prime number database
        bool enable_logging = true;          // Enable performance logging
        bool store_primes = true;            // Store discovered primes in database
        size_t cache_size_mb = 128;          // Memory cache size in MB
    };
    
    // Constructor and destructor
    MFPSystem(const Config& config = Config());
    ~MFPSystem();
    
    // Initialization
    bool initialize();
    bool isInitialized() const;
    
    // Core operations
    bool isPrime(const mpz_t& number);
    bool findDivisor(const mpz_t& number, mpz_t& divisor_out);
    bool factorize(const mpz_t& number, std::vector<mpz_t>& factors_out);
    
    // Batch operations
    bool checkPrimes(const std::vector<mpz_t>& numbers, std::vector<bool>& results_out);
    bool findDivisors(const std::vector<mpz_t>& numbers, std::vector<mpz_t>& divisors_out);
    
    // Database operations
    bool storePrime(const mpz_t& prime);
    bool getPrimesInRange(const mpz_t& min_value, const mpz_t& max_value, 
                         std::vector<mpz_t>& primes_out, uint64_t max_results = 1000);
    uint64_t getTotalPrimes() const;
    uint64_t getDatabaseSize() const;
    
    // Performance monitoring
    PerformanceReport getPerformanceReport() const;
    void resetPerformanceMetrics();
    void setPerformanceLogging(bool enable);
    
    // Configuration
    void setMethod(int method);
    void setNumThreads(int num_threads);
    void setDatabasePath(const std::string& path);
    void setCacheSize(size_t size_mb);
    
    // System information
    struct SystemInfo {
        int method;
        int num_threads;
        std::string db_path;
        bool logging_enabled;
        bool store_primes;
        size_t cache_size_mb;
        uint64_t total_primes;
        uint64_t database_size;
        double average_execution_time;
        double parallel_efficiency;
    };
    
    SystemInfo getSystemInfo() const;
    
private:
    // Component instances
    std::unique_ptr<MFPBase> mfp_engine_;
    std::unique_ptr<PrimeDatabase> prime_db_;
    std::unique_ptr<PerformanceMonitor> perf_monitor_;
    std::unique_ptr<ParallelFramework> parallel_framework_;
    
    // Configuration
    Config config_;
    
    // State
    bool initialized_;
    
    // Helper methods
    std::unique_ptr<MFPBase> createMFPEngine(int method, int num_threads);
};

} // namespace mfp
