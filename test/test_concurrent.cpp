#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <future>
#include <iomanip>
#include <fstream>
#include <sstream>
#include "../include/mfp_method1.h"
#include "../include/mfp_method2.h"
#include "../include/mfp_method3.h"

// Structure to hold performance metrics
struct PerformanceMetrics {
    std::string methodName;
    std::string number;
    double duration;
    int threadCount;
    std::vector<std::string> factors;
    std::thread::id threadId;
};

// Function to run Method 1 in a separate thread
PerformanceMetrics runMethod1(const std::string& number) {
    PerformanceMetrics metrics;
    metrics.methodName = "Method 1 (Expanded q Factorization)";
    metrics.number = number;
    metrics.threadCount = 1;
    metrics.threadId = std::this_thread::get_id();
    
    std::cout << "Starting " << metrics.methodName << " on thread " 
              << metrics.threadId << std::endl;
    
    mfp::MFPMethod1 method1;
    
    // Measure CPU time
    clock_t cpu_start = clock();
    
    // Measure wall clock time
    auto start = std::chrono::high_resolution_clock::now();
    metrics.factors = method1.factorize(number);
    auto end = std::chrono::high_resolution_clock::now();
    
    // Calculate durations
    metrics.duration = std::chrono::duration<double>(end - start).count();
    double cpu_duration = static_cast<double>(clock() - cpu_start) / CLOCKS_PER_SEC;
    
    std::cout << metrics.methodName << " completed in " << metrics.duration 
              << " seconds (wall clock), " << cpu_duration << " seconds (CPU time)" << std::endl;
    
    return metrics;
}

// Function to run Method 2 in a separate thread
PerformanceMetrics runMethod2(const std::string& number) {
    PerformanceMetrics metrics;
    metrics.methodName = "Method 2 (Ultrafast with Structural Filter)";
    metrics.number = number;
    metrics.threadCount = 1;
    metrics.threadId = std::this_thread::get_id();
    
    std::cout << "Starting " << metrics.methodName << " on thread " 
              << metrics.threadId << std::endl;
    
    mfp::MFPMethod2 method2;
    
    // Measure CPU time
    clock_t cpu_start = clock();
    
    // Measure wall clock time
    auto start = std::chrono::high_resolution_clock::now();
    metrics.factors = method2.factorize(number);
    auto end = std::chrono::high_resolution_clock::now();
    
    // Calculate durations
    metrics.duration = std::chrono::duration<double>(end - start).count();
    double cpu_duration = static_cast<double>(clock() - cpu_start) / CLOCKS_PER_SEC;
    
    std::cout << metrics.methodName << " completed in " << metrics.duration 
              << " seconds (wall clock), " << cpu_duration << " seconds (CPU time)" << std::endl;
    
    return metrics;
}

// Function to run Method 3 in a separate thread
PerformanceMetrics runMethod3(const std::string& number, int numThreads) {
    PerformanceMetrics metrics;
    metrics.methodName = "Method 3 (Parallelized with Dynamic Blocks)";
    metrics.number = number;
    metrics.threadCount = numThreads;
    metrics.threadId = std::this_thread::get_id();
    
    std::cout << "Starting " << metrics.methodName << " with " 
              << numThreads << " threads on thread " 
              << metrics.threadId << std::endl;
    
    mfp::MFPMethod3 method3(numThreads);
    
    // Measure CPU time
    clock_t cpu_start = clock();
    
    // Measure wall clock time
    auto start = std::chrono::high_resolution_clock::now();
    metrics.factors = method3.factorize(number);
    auto end = std::chrono::high_resolution_clock::now();
    
    // Calculate durations
    metrics.duration = std::chrono::duration<double>(end - start).count();
    double cpu_duration = static_cast<double>(clock() - cpu_start) / CLOCKS_PER_SEC;
    
    std::cout << metrics.methodName << " completed in " << metrics.duration 
              << " seconds (wall clock), " << cpu_duration << " seconds (CPU time)" << std::endl;
    
    return metrics;
}

// Function to print factors
void printFactors(const PerformanceMetrics& metrics) {
    std::cout << metrics.methodName << " found factors: ";
    for (size_t i = 0; i < metrics.factors.size(); ++i) {
        std::cout << metrics.factors[i];
        if (i < metrics.factors.size() - 1) std::cout << ", ";
    }
    std::cout << std::endl;
}

// Function to save performance metrics to CSV file
void saveMetricsToCSV(const std::vector<PerformanceMetrics>& allMetrics, const std::string& filename) {
    std::ofstream file(filename);
    
    // Write header
    file << "Number,Method,ThreadCount,Duration,FactorCount,Factors,ThreadID" << std::endl;
    
    // Write data
    for (const auto& metrics : allMetrics) {
        std::stringstream factors;
        for (size_t i = 0; i < metrics.factors.size(); ++i) {
            factors << metrics.factors[i];
            if (i < metrics.factors.size() - 1) factors << " × ";
        }
        
        file << metrics.number << ","
             << metrics.methodName << ","
             << metrics.threadCount << ","
             << metrics.duration << ","
             << metrics.factors.size() << ","
             << "\"" << factors.str() << "\","
             << metrics.threadId
             << std::endl;
    }
    
    file.close();
    std::cout << "Performance metrics saved to " << filename << std::endl;
}

// Function to print performance comparison table
void printPerformanceTable(const std::vector<PerformanceMetrics>& allMetrics) {
    std::cout << "\n=== Performance Comparison Table ===\n" << std::endl;
    
    // Print header
    std::cout << std::left << std::setw(15) << "Number" 
              << std::setw(45) << "Method" 
              << std::setw(15) << "Threads" 
              << std::setw(15) << "Duration (s)" 
              << std::setw(15) << "Speedup" 
              << std::endl;
    
    std::cout << std::string(105, '-') << std::endl;
    
    // Group metrics by number
    std::string currentNumber = "";
    double baselineDuration = 0.0;
    
    for (const auto& metrics : allMetrics) {
        if (metrics.number != currentNumber) {
            currentNumber = metrics.number;
            
            // Find Method 1 duration for this number to use as baseline
            for (const auto& m : allMetrics) {
                if (m.number == currentNumber && m.methodName.find("Method 1") != std::string::npos) {
                    baselineDuration = m.duration;
                    break;
                }
            }
            
            // Print number as a header
            std::cout << "\nNumber: " << currentNumber << std::endl;
            std::cout << std::string(105, '-') << std::endl;
        }
        
        // Calculate speedup relative to Method 1
        double speedup = baselineDuration / metrics.duration;
        
        // Print metrics row
        std::cout << std::left << std::setw(15) << "" 
                  << std::setw(45) << metrics.methodName 
                  << std::setw(15) << metrics.threadCount 
                  << std::setw(15) << std::fixed << std::setprecision(6) << metrics.duration 
                  << std::setw(15) << std::fixed << std::setprecision(2) << speedup << "x" 
                  << std::endl;
    }
    
    std::cout << std::endl;
}

int main() {
    // Print system information
    std::cout << "System has " << std::thread::hardware_concurrency() 
              << " hardware threads available" << std::endl;
    std::cout << "Main thread ID: " << std::this_thread::get_id() << std::endl;
    
    // Test numbers of increasing size
    std::vector<std::string> numbers = {
        "91",                      // Small: 7 × 13
        "1522605027922533",        // Medium: 1234567 × 1234567
        "9007199254740991",        // Large: 6361 × 1416003655831
        "147573952589676412927"    // Very large: 193707721 × 761838257287
    };
    
    // Vector to store all performance metrics
    std::vector<PerformanceMetrics> allMetrics;
    
    for (const auto& number : numbers) {
        std::cout << "\n========================================" << std::endl;
        std::cout << "Testing with number: " << number << std::endl;
        std::cout << "========================================\n" << std::endl;
        
        // Start all three methods concurrently
        auto future1 = std::async(std::launch::async, runMethod1, number);
        auto future2 = std::async(std::launch::async, runMethod2, number);
        auto future3 = std::async(std::launch::async, runMethod3, number, 28); // 28 cores for Method 3
        
        // Wait for all methods to complete and get results
        auto metrics1 = future1.get();
        auto metrics2 = future2.get();
        auto metrics3 = future3.get();
        
        // Store metrics
        allMetrics.push_back(metrics1);
        allMetrics.push_back(metrics2);
        allMetrics.push_back(metrics3);
        
        // Print results
        std::cout << "\nResults for number " << number << ":" << std::endl;
        printFactors(metrics1);
        printFactors(metrics2);
        printFactors(metrics3);
        
        std::cout << "\n----------------------------------------\n" << std::endl;
    }
    
    // Print performance comparison table
    printPerformanceTable(allMetrics);
    
    // Save metrics to CSV file
    saveMetricsToCSV(allMetrics, "mfp_performance_metrics.csv");
    
    return 0;
}
