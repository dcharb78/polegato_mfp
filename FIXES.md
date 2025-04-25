# Repository Fixes Documentation

## Issues Identified

1. **Hardcoded Paths**: The main issue with the repository was hardcoded paths in the build directory. The CMake build system had cached absolute paths to "/home/ubuntu/mfp_test" instead of using relative paths, which prevented the repository from working when cloned to a different location.

2. **Missing Dependencies**: The repository required several dependencies that weren't explicitly documented:
   - CMake build system
   - C++ compiler (g++)
   - GMP development library (libgmp-dev)

## Fixes Applied

1. **Removed Hardcoded Build Directory**: 
   - Deleted the existing build directory that contained hardcoded paths
   - Created a fresh build directory with proper relative paths

2. **Installed Required Dependencies**:
   - Installed CMake build system
   - Installed C++ compiler (g++)
   - Installed GMP development library (libgmp-dev)

3. **Rebuilt the Project**:
   - Successfully configured the project with CMake
   - Successfully built the project with make

## How to Use the Repository

To clone and build this repository:

```bash
# Clone the repository
git clone https://github.com/dcharb78/polegato_mfp.git

# Install required dependencies (Ubuntu/Debian)
sudo apt-get update
sudo apt-get install -y cmake g++ libgmp-dev

# Create build directory and build the project
cd polegato_mfp
mkdir -p build
cd build
cmake ..
make
```

## Notes

- The original repository had a build directory committed with hardcoded paths to "/home/ubuntu/mfp_test"
- It's generally not recommended to commit build directories to version control
- A .gitignore file could be added to prevent accidentally committing build artifacts in the future
