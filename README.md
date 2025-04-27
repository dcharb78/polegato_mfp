# Modular Factorization Pattern (MFP) Implementation

This package contains a complete implementation of the Modular Factorization Pattern (MFP) as described in the specification document. The implementation includes three methods for factorizing large numbers:

1. **Method 1: Expanded q Factorization** - A single-threaded implementation that uses the decimal redistribution pattern with q_max = A^(2/3).

2. **Method 2: Ultrafast with Structural Filter** - A single-threaded implementation that adds a structural filter (A - i) % d == 0 to improve performance.

3. **Method 3: Parallelized with Dynamic Blocks** - A multi-threaded implementation that divides the search space into blocks that can be processed in parallel.

## Requirements

- C++11 compiler
- GMP (GNU Multiple Precision Arithmetic Library)
- CMake (version 3.10 or higher)
- pthread support

## Building the Project

```bash
mkdir -p build
cd build
cmake ..
make
```

## Running the Tests

The project includes several test programs to verify the implementation:

### Basic Test

Tests all three methods with a variety of numbers and verifies the results:

```bash
./test_mfp
```

### Prime Number Test

Tests all three methods with a known large prime number to verify they correctly identify it as prime:

```bash
./test_prime
```

### Composite Number Test

Tests all three methods with composite numbers that have known factors:

```bash
./test_composite
```

### Concurrent Test

Runs all three methods simultaneously, with 1 core for non-threaded methods and 28 cores for the threaded method:

```bash
./test_concurrent
```

This test also generates performance metrics and saves them to a CSV file.

## Using the CLI Interface

The project now includes a command-line interface (CLI) that allows you to factorize a single number or a range of numbers using any of the three methods:

```bash
./mfp_cli [OPTIONS]
```

Options:
- `-n, --number NUMBER` - Specify a single number to factorize
- `-r, --range START END` - Specify a range of numbers to factorize
- `-m, --method METHOD` - Specify the factorization method (1, 2, or 3, default: 3)
- `-c, --cpu COUNT` - Specify the number of CPU cores to use (default: auto)
- `-h, --help` - Display help message

Examples:
```bash
# Factorize a single number using method 3 (default)
./mfp_cli --number 123456789

# Factorize a single number using method 1
./mfp_cli --number 123456789 --method 1

# Factorize a single number using method 3 with 4 CPU cores
./mfp_cli --number 123456789 --method 3 --cpu 4

# Factorize a range of numbers using method 2
./mfp_cli --range 100 200 --method 2
```

## Using the Shell Script Interface

For convenience, a shell script interface is provided that simplifies the usage of the CLI:

```bash
./mfp.sh [OPTIONS] NUMBER|RANGE
```

Options:
- `-m METHOD` - Specify the factorization method (1, 2, or 3, default: 3)
- `-c CPU` - Specify the number of CPU cores to use (default: auto)
- `-h` - Display help message

Examples:
```bash
# Factorize a single number using default method and CPU count
./mfp.sh 123456789

# Factorize a single number using method 1
./mfp.sh -m 1 123456789

# Factorize a single number using method 2 with 4 CPU cores
./mfp.sh -m 2 -c 4 123456789

# Factorize numbers in range 100 to 200 using default method
./mfp.sh 100-200

# Factorize numbers in range 1000 to 1010 using method 3 with 8 CPU cores
./mfp.sh -m 3 -c 8 1000-1010
```

## Implementation Details

### Method 1: Expanded q Factorization

This method implements the basic MFP approach using the formula:
- nk = 10*A + d0 where k ∈ {1, 3, 7, 9}
- i = (A - qd0)/(10q + 1)
- d = d0 + 10*i

### Method 2: Ultrafast with Structural Filter

This method extends Method 1 by adding a structural filter:
- Check if (A - i) % d == 0 before testing if d is a divisor of n

### Method 3: Parallelized with Dynamic Blocks

This method divides the search space into blocks that can be processed in parallel:
- Allocates 2/3 of threads to i-search and 1/3 to q-sweep for optimal performance
- Uses dynamic block sizing to ensure efficient work distribution
- Implements an alternating pattern for i-search above and below i_est

## Performance

Method 3 provides the best performance, especially for large numbers, as it can utilize multiple CPU cores. The performance improvement scales with the number of available cores.

## License

Copyright (c) 2025 Daniel Charboneau. All rights reserved.
