#pragma once

#include "mfp_base.h"
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <queue>

namespace mfp {

class MFPMethod3 : public MFPBase {
public:
    // Constructor and destructor
    MFPMethod3(int num_threads = 32);
    ~MFPMethod3() override;
    
    // Core methods implementation
    bool isPrime(const mpz_t& number) override;
    bool findDivisor(const mpz_t& number, mpz_t& divisor_out) override;
    
private:
    // Method-specific parameters
    int num_threads_;
    
    // Thread management
    std::vector<std::thread> worker_threads_;
    std::atomic<bool> shutdown_;
    
    // Task queue
    struct Task {
        enum class Type { PRIMALITY_TEST, FIND_DIVISOR };
        Type type;
        mpz_t number;
        mpz_t result;
        bool completed;
        bool success;
        std::mutex mutex;
        std::condition_variable cv;
    };
    
    std::queue<std::shared_ptr<Task>> task_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    
    // Dynamic block management
    struct Block {
        mpz_t start;
        mpz_t end;
        int priority;
    };
    
    // Method-specific helper methods
    void workerThread();
    void initializeThreads();
    void shutdownThreads();
    
    std::shared_ptr<Task> submitTask(Task::Type type, const mpz_t& number);
    void waitForTask(std::shared_ptr<Task> task);
    
    bool parallelizedFactorization(const mpz_t& number, mpz_t& divisor_out);
    std::vector<Block> createDynamicBlocks(const mpz_t& number);
    bool processBlock(const Block& block, mpz_t& divisor_out);
};

} // namespace mfp
