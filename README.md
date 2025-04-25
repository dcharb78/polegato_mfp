# MFP Implementation

This repository contains an implementation of the Modular Factorization Pattern (MFP) algorithm created by [Marlon F. Polegato](https://www.linkedin.com/in/marlonpolegato/). The implementation includes automatic hardware detection, GPU acceleration, and dynamic resource allocation to optimize performance across different hardware configurations.

## Attribution

The Modular Factorization Pattern (MFP) algorithm was created by **Marlon F. Polegato**. This repository only packages his algorithm for testing purposes with enhanced features for hardware detection and GPU acceleration.

## Features

- **Three MFP Methods**:
  - Method 1: Expanded q Factorization
  - Method 2: Ultrafast with Structural Filter
  - Method 3: Parallelized with Dynamic Blocks

- **Automatic Hardware Detection**:
  - CPU detection (architecture, cores, features)
  - Memory and storage detection
  - GPU detection for both NVIDIA and Apple hardware

- **GPU Acceleration**:
  - CUDA implementation for NVIDIA GPUs
  - Metal implementation for Apple GPUs
  - Optimized kernels for all three MFP methods

- **Dynamic Resource Allocation**:
  - Automatic selection of optimal hardware
  - Multiple execution strategies (CPU, CUDA, Metal, Hybrid)
  - Performance benchmarking for strategy selection

- **Performance Metrics**:
  - Execution time measurement
  - Memory usage tracking
  - Thread utilization monitoring
  - Detailed performance reports

## Requirements

- C++17 compatible compiler
- CMake 3.10 or higher
- GMP (GNU Multiple Precision Arithmetic Library)
- CUDA Toolkit 11.0+ (optional, for NVIDIA GPU acceleration)
- Metal framework (optional, for Apple GPU acceleration)

## Building

```bash
# Clone the repository
git clone https://github.com/dcharb78/polegato_mfp.git
cd polegato_mfp

# Create build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build
make
```

## Usage

### Command-Line Interface

```bash
# Check if a number is prime
./mfp_app isprime 104729

# Factorize a number
./mfp_app factorize 123456789

# Find the next prime after a number
./mfp_app nextprime 104729

# Run a benchmark with 2000-bit numbers
./mfp_app benchmark 2000

# Display system information
./mfp_app sysinfo

# Use Method 3 with CUDA acceleration
./mfp_app isprime 104729 --method 3 --strategy cuda
```

### API Usage

```cpp
#include "mfp_system.h"
#include <gmp.h>

int main() {
    // Initialize the system
    mfp::getResourceManager().initialize();
    mfp::getConfigurationManager().initialize(mfp::getResourceManager());
    
    // Create MFP implementation (automatically selects best method and strategy)
    auto mfp = mfp::getResourceManager().createMFP(1);
    
    // Enable performance metrics
    mfp->enablePerformanceMetrics(true);
    
    // Check if a number is prime
    mpz_t number;
    mpz_init_set_str(number, "104729", 10);
    
    bool is_prime;
    mfp->isPrime(number, is_prime);
    
    std::cout << "Is prime: " << (is_prime ? "Yes" : "No") << std::endl;
    
    // Get performance metrics
    const mfp::PerformanceMetrics& metrics = mfp->getPerformanceMetrics();
    std::cout << "Execution time: " << metrics.total_execution_time_ms << " ms" << std::endl;
    
    // Clean up
    mpz_clear(number);
    
    return 0;
}
```

## Documentation

For detailed documentation, see the following files:

- [Enhanced Capabilities](docs/enhanced_capabilities.md): Comprehensive documentation of the enhanced features
- [Original MFP Paper](Modular%20Factorization%20Pattern%20M.F.P.pdf): The original paper describing the MFP algorithm

## License

This project is licensed under the Creative Commons Attribution-NonCommercial 4.0 International License - see the [LICENSE](LICENSE) file for details.

This license allows free use for academic and research purposes but prohibits commercial use. Attribution to Marlon F. Polegato is required for any use of this work.

## Acknowledgements

- Marlon F. Polegato for creating the MFP algorithm
- The GMP team for the GNU Multiple Precision Arithmetic Library
- NVIDIA for the CUDA toolkit
- Apple for the Metal framework
