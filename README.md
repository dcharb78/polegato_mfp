# Enhanced Modular Factorization Pattern (MFP) Implementation

This repository contains an enhanced implementation of the Modular Factorization Pattern (MFP) algorithm created by [Marlon F. Polegato](https://www.linkedin.com/in/marlonpolegato/). The implementation includes automatic hardware detection, GPU acceleration, and dynamic configuration features.

## Overview

The Modular Factorization Pattern (MFP) is a mathematical algorithm for prime number identification and factorization. This enhanced implementation builds upon the original algorithm with the following improvements:

- **Automatic Hardware Detection**: Detects CPU, RAM, and GPU capabilities
- **GPU Acceleration**: Supports both NVIDIA CUDA and Apple Metal GPUs
- **Dynamic Resource Allocation**: Automatically utilizes the best available hardware
- **Automatic Configuration**: Optimizes settings based on detected hardware
- **Performance Logging**: Comprehensive metrics for performance analysis
- **Prime Number Database**: Efficient storage and retrieval of prime numbers

The implementation is designed to run efficiently on a wide range of hardware, from low-end laptops to high-end servers with 32+ cores and multiple GPUs.

## Features

- **Three MFP Variants**:
  - Method 1: Expanded q Factorization
  - Method 2: Ultrafast with Structural Filter
  - Method 3: Parallelized with Dynamic Blocks

- **Hardware Support**:
  - Multi-core CPU utilization
  - NVIDIA GPU acceleration via CUDA
  - Apple GPU acceleration via Metal
  - Hybrid CPU/GPU execution

- **Performance Optimization**:
  - Automatic thread count optimization
  - Dynamic workload distribution
  - Memory usage optimization
  - Cache-aware algorithms

- **Configuration System**:
  - Hardware-specific profiles
  - Parameter management
  - Configuration persistence
  - Dynamic reconfiguration

## Requirements

- C++17 compatible compiler
- CMake 3.12 or higher
- GMP (GNU Multiple Precision Arithmetic Library)
- For CUDA support: CUDA Toolkit 11.0 or higher
- For Metal support: macOS 10.15 or higher with Xcode

## Building

```bash
# Clone the repository
git clone https://github.com/yourusername/mfp-enhanced.git
cd mfp-enhanced

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build
make

# Run tests
make test
```

## Usage

### Basic Usage

```cpp
#include "mfp_system.h"

int main() {
    // Initialize the MFP system
    mfp::MFPSystem system;
    system.initialize();
    
    // Check if a number is prime
    std::string number = "1234567890123456789";
    bool is_prime = system.isPrime(number);
    
    // Find prime factors
    std::vector<std::string> factors;
    bool success = system.findPrimeFactors(number, factors);
    
    // Print factors
    if (success) {
        for (const auto& factor : factors) {
            std::cout << factor << std::endl;
        }
    }
    
    return 0;
}
```

### Advanced Usage

See the [documentation](docs/enhanced_capabilities.md) for advanced usage examples and API reference.

## Documentation

- [Enhanced Capabilities](docs/enhanced_capabilities.md): Comprehensive documentation of the enhanced system
- [System Architecture](docs/system_architecture.md): Detailed description of the system architecture
- [API Reference](docs/api_reference.md): Complete API reference
- [Performance Guide](docs/performance_guide.md): Guide to optimizing performance
- [Original MFP Paper](docs/Modular%20Factorization%20Pattern%20M.F.P.pdf): The original MFP paper by Marlon F. Polegato

## License

This project is licensed under the Creative Commons Attribution-NonCommercial 4.0 International License (CC BY-NC 4.0).

- You are free to use this software for academic and research purposes
- Commercial use is prohibited without explicit permission
- Attribution to Marlon F. Polegato as the creator of the MFP algorithm is required

## Acknowledgements

- Marlon F. Polegato for creating the original Modular Factorization Pattern algorithm
- The GMP development team for the GNU Multiple Precision Arithmetic Library
- The CUDA and Metal development teams for their GPU computing frameworks
