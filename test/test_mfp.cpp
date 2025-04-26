#include <gmp.h>
#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include "../include/mfp_method1.h"
#include "../include/mfp_method2.h"
#include "../include/mfp_method3.h"

using namespace std;
using namespace mfp;
using clk = std::chrono::high_resolution_clock;

int main() {
    // Test numbers from the PDF (excluding the extremely large one)
    vector<string> numbers = {
        "91",
        "15",
        "2199023255551",
        "9007199254740991",
        "147573952589676412927"
    };

    // Test Method 1: Expanded q Factorization
    cout << "\n=== Testing Method 1: Expanded q Factorization ===\n";
    MFPMethod1 method1;
    
    for (const string &snum : numbers) {
        cout << "\nNumber: " << snum << "\n";
        
        auto t0 = clk::now();
        vector<string> factors = method1.factorize(snum);
        auto t1 = clk::now();
        
        if (factors.size() == 1 && factors[0] == snum) {
            cout << " No divisor (prime)\n";
        } else {
            cout << " Factors: ";
            for (size_t i = 0; i < factors.size(); ++i) {
                cout << factors[i];
                if (i < factors.size() - 1) cout << ", ";
            }
            cout << "\n";
        }
        
        double dt = std::chrono::duration<double>(t1 - t0).count();
        cout << " Time: " << dt << " s\n";
        cout << "-----------------------------\n";
    }
    
    // Test Method 2: Ultrafast with Structural Filter
    cout << "\n=== Testing Method 2: Ultrafast with Structural Filter ===\n";
    MFPMethod2 method2;
    
    for (const string &snum : numbers) {
        cout << "\nNumber: " << snum << "\n";
        
        auto t0 = clk::now();
        vector<string> factors = method2.factorize(snum);
        auto t1 = clk::now();
        
        if (factors.size() == 1 && factors[0] == snum) {
            cout << " No divisor (prime)\n";
        } else {
            cout << " Factors: ";
            for (size_t i = 0; i < factors.size(); ++i) {
                cout << factors[i];
                if (i < factors.size() - 1) cout << ", ";
            }
            cout << "\n";
        }
        
        double dt = std::chrono::duration<double>(t1 - t0).count();
        cout << " Time: " << dt << " s\n";
        cout << "-----------------------------\n";
    }
    
    // Test Method 3: Parallelized with Dynamic Blocks
    cout << "\n=== Testing Method 3: Parallelized with Dynamic Blocks ===\n";
    MFPMethod3 method3(8); // Use 8 threads as specified in the PDF
    
    for (const string &snum : numbers) {
        cout << "\nNumber: " << snum << "\n";
        
        auto t0 = clk::now();
        vector<string> factors = method3.factorize(snum);
        auto t1 = clk::now();
        
        if (factors.size() == 1 && factors[0] == snum) {
            cout << " No divisor (prime)\n";
        } else {
            cout << " Factors: ";
            for (size_t i = 0; i < factors.size(); ++i) {
                cout << factors[i];
                if (i < factors.size() - 1) cout << ", ";
            }
            cout << "\n";
        }
        
        double dt = std::chrono::duration<double>(t1 - t0).count();
        cout << " Time: " << dt << " s\n";
        cout << "-----------------------------\n";
    }
    
    return 0;
}
