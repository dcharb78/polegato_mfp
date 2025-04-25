#pragma once

#include <string>
#include <vector>
#include <memory>
#include <gmp.h>
#include <mutex>
#include <atomic>
#include <fstream>

namespace mfp {

class PrimeDatabase {
public:
    // Configuration options
    struct Config {
        std::string db_path = "./primes_db";
        uint32_t cache_size_mb = 128;
        uint32_t max_threads = 8;
        bool enable_compression = true;
        bool create_if_missing = true;
        bool read_only = false;
    };
    
    // Constructor and destructor
    PrimeDatabase(const Config& config = Config());
    ~PrimeDatabase();
    
    // Initialization
    bool open();
    void close();
    bool isOpen() const;
    
    // Basic operations
    bool storePrime(const mpz_t& prime);
    bool isPrime(const mpz_t& number);
    
    // Batch operations
    bool storePrimes(const std::vector<mpz_t>& primes);
    bool checkPrimes(const std::vector<mpz_t>& numbers, std::vector<bool>& results_out);
    
    // Range queries
    bool getPrimesInRange(const mpz_t& min_value, const mpz_t& max_value, 
                         std::vector<mpz_t>& primes_out,
                         uint64_t max_results = 1000);
    
    // Statistics
    uint64_t getTotalPrimes() const;
    uint64_t getDatabaseSize() const;
    uint64_t getMaxPrimeDigits() const;
    
    // Configuration
    void setCacheSize(size_t size_mb);
    
    // Maintenance
    bool compact();
    bool backup(const std::string& backup_path);
    bool verify();
    
private:
    // Internal components
    class StorageEngine;
    class IndexSystem;
    class CompressionSystem;
    
    std::unique_ptr<StorageEngine> storage_;
    std::unique_ptr<IndexSystem> index_;
    std::unique_ptr<CompressionSystem> compression_;
    
    // Configuration
    Config config_;
    
    // State
    bool is_open_;
    mutable std::mutex state_mutex_;
    
    // Worker thread pool
    class ThreadPool;
    std::unique_ptr<ThreadPool> thread_pool_;
};

} // namespace mfp
