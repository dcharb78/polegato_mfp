#include <gmp.h>
#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <iomanip>
#include "fixed_mfp/include/mfp_method1.h"
#include "fixed_mfp/include/mfp_method2.h"
#include "fixed_mfp/include/mfp_method3.h"

using namespace std;
using namespace mfp;
using clk = std::chrono::high_resolution_clock;

// Convert hex string to decimal string
string hex_to_decimal(const string& hex) {
    mpz_t n;
    mpz_init(n);
    
    // Remove 0x prefix if present
    string clean_hex = hex;
    if (clean_hex.substr(0, 2) == "0x") {
        clean_hex = clean_hex.substr(2);
    }
    
    // Convert hex to mpz
    mpz_set_str(n, clean_hex.c_str(), 16);
    
    // Convert mpz to decimal string
    char* decimal = mpz_get_str(nullptr, 10, n);
    string result(decimal);
    
    // Clean up
    free(decimal);
    mpz_clear(n);
    
    return result;
}

int main() {
    // RSA modulus in hex format
    string modulus_hex = "D32C3FE402D0F305E53FD901A936528ED1DC74255F9B47E74A2654D5152288090AA8BDC49FFDAD16ABC718D921DE5EA803AF4F60DC52275F8D7B2EF420BA7729";
    
    // Convert to decimal
    string modulus_decimal = hex_to_decimal(modulus_hex);
    
    cout << "RSA Key (512-bit) Factorization Test\n";
    cout << "====================================\n\n";
    cout << "Modulus (hex): " << modulus_hex << "\n";
    cout << "Modulus (decimal): " << modulus_decimal << "\n\n";
    
    // Test with Method 1: Expanded q Factorization
    cout << "Attempting factorization with Method 1 (Expanded q Factorization)...\n";
    MFPMethod1 method1;
    
    auto t0 = clk::now();
    vector<string> factors1 = method1.factorize(modulus_decimal);
    auto t1 = clk::now();
    
    double dt1 = std::chrono::duration<double>(t1 - t0).count();
    cout << "Time: " << dt1 << " s\n";
    
    if (factors1.size() > 1) {
        cout << "Factors found:\n";
        for (const auto& factor : factors1) {
            cout << factor << "\n";
        }
    } else {
        cout << "No factors found with Method 1\n";
    }
    cout << "\n";
    
    // Test with Method 2: Ultrafast with Structural Filter
    cout << "Attempting factorization with Method 2 (Ultrafast with Structural Filter)...\n";
    MFPMethod2 method2;
    
    t0 = clk::now();
    vector<string> factors2 = method2.factorize(modulus_decimal);
    t1 = clk::now();
    
    double dt2 = std::chrono::duration<double>(t1 - t0).count();
    cout << "Time: " << dt2 << " s\n";
    
    if (factors2.size() > 1) {
        cout << "Factors found:\n";
        for (const auto& factor : factors2) {
            cout << factor << "\n";
        }
    } else {
        cout << "No factors found with Method 2\n";
    }
    cout << "\n";
    
    // Test with Method 3: Parallelized with Dynamic Blocks
    cout << "Attempting factorization with Method 3 (Parallelized with Dynamic Blocks)...\n";
    MFPMethod3 method3(8); // Use 8 threads as specified in the PDF
    
    t0 = clk::now();
    vector<string> factors3 = method3.factorize(modulus_decimal);
    t1 = clk::now();
    
    double dt3 = std::chrono::duration<double>(t1 - t0).count();
    cout << "Time: " << dt3 << " s\n";
    
    if (factors3.size() > 1) {
        cout << "Factors found:\n";
        for (const auto& factor : factors3) {
            cout << factor << "\n";
        }
    } else {
        cout << "No factors found with Method 3\n";
    }
    
    return 0;
}
