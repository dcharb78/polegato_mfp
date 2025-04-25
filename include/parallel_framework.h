#pragma once

#include <vector>
#include <thread>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <future>
#include <memory>

namespace mfp {

class ParallelFramework {
public:
    // Constructor and destructor
    ParallelFramework(int num_threads = 32);
    ~ParallelFramework();
    
    // Task submission
    template<class F, class... Args>
    auto submitTask(F&& f, Args&&... args) 
        -> std::future<typename std::result_of<F(Args...)>::type>;
    
    // Batch task submission
    template<class F, class... Args>
    auto submitBatchTasks(size_t count, F&& f, Args&&... args)
        -> std::vector<std::future<typename std::result_of<F(Args...)>::type>>;
    
    // Control
    void waitForAll();
    void setNumThreads(int num_threads);
    int getNumThreads() const;
    
    // Performance metrics
    struct Metrics {
        double speedup;
        double efficiency;
        double load_balance;
        double overhead;
    };
    
    Metrics getMetrics() const;
    
private:
    // Thread pool
    class ThreadPool;
    std::unique_ptr<ThreadPool> thread_pool_;
    
    // Work scheduler
    class WorkScheduler;
    std::unique_ptr<WorkScheduler> work_scheduler_;
    
    // Load balancer
    class LoadBalancer;
    std::unique_ptr<LoadBalancer> load_balancer_;
    
    // Synchronization manager
    class SynchronizationManager;
    std::unique_ptr<SynchronizationManager> sync_manager_;
    
    // Memory manager
    class MemoryManager;
    std::unique_ptr<MemoryManager> memory_manager_;
    
    // Configuration
    int num_threads_;
    
    // Performance metrics
    mutable std::mutex metrics_mutex_;
    Metrics metrics_;
};

// Template implementation
template<class F, class... Args>
auto ParallelFramework::submitTask(F&& f, Args&&... args) 
    -> std::future<typename std::result_of<F(Args...)>::type> {
    
    return thread_pool_->enqueue(std::forward<F>(f), std::forward<Args>(args)...);
}

template<class F, class... Args>
auto ParallelFramework::submitBatchTasks(size_t count, F&& f, Args&&... args)
    -> std::vector<std::future<typename std::result_of<F(Args...)>::type>> {
    
    using return_type = typename std::result_of<F(Args...)>::type;
    std::vector<std::future<return_type>> futures;
    futures.reserve(count);
    
    for (size_t i = 0; i < count; ++i) {
        futures.push_back(submitTask(std::forward<F>(f), std::forward<Args>(args)...));
    }
    
    return futures;
}

} // namespace mfp
