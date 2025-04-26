# Modular Factorization Pattern (MFP) Implementation

This repository contains a fixed implementation of the Modular Factorization Pattern (MFP) as described in the accompanying PDF document. The MFP is a novel approach to integer factorization based on decimal redistribution patterns.

## Overview

The implementation includes three methods of the MFP:

1. **Method 1: Expanded q Factorization** - Uses a sweep of q values up to a limit based on A^(2/3)
2. **Method 2: Ultrafast with Structural Filter** - Similar to Method 1 but with an additional structural filter
3. **Method 3: Parallelized with Dynamic Blocks** - Divides the search into blocks with parallel processing

All three methods share a common mathematical foundation based on the formula:
- `nk = n * k` where k ∈ {1, 3, 7, 9}
- `A = floor(nk / 10)`
- `d0 = nk % 10`
- `d = d0 + 10*i`
- `i = (A - qd0)/(10q + 1)`

## Requirements

- C++11 or later
- GMP (GNU Multiple Precision Arithmetic Library)
- CMake 3.10 or later
- pthread support

## Building

```bash
mkdir -p build
cd build
cmake ..
make
```

## Testing

After building, run the test executables:

```bash
# Main test with numbers from the PDF
./test_mfp

# Test with a known large prime number
./test_prime

# Test with a composite number that has known factors
./test_composite
```

## Usage

The library provides three classes for factorization:

- `MFPMethod1` - Implements the Expanded q Factorization method
- `MFPMethod2` - Implements the Ultrafast with Structural Filter method
- `MFPMethod3` - Implements the Parallelized with Dynamic Blocks method

Example usage:

```cpp
#include "include/mfp_method1.h"
#include <iostream>
#include <vector>
#include <string>

int main() {
    mfp::MFPMethod1 method;
    std::string number = "9007199254740991";
    
    std::vector<std::string> factors = method.factorize(number);
    
    std::cout << "Factors of " << number << ":" << std::endl;
    for (const auto& factor : factors) {
        std::cout << factor << std::endl;
    }
    
    return 0;
}
```

## Performance

The three methods have different performance characteristics:

- **Method 1** is suitable for general-purpose factorization
- **Method 2** is faster for large numbers due to the additional structural filter
- **Method 3** provides the best performance for very large numbers by utilizing parallel processing

## Author

Daniel Charboneau
