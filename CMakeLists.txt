cmake_minimum_required(VERSION 3.10)
project(fixed_mfp)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find GMP package
find_library(GMP_LIBRARIES NAMES gmp)
if(NOT GMP_LIBRARIES)
    message(FATAL_ERROR "GMP library not found")
endif()

# Include directories
include_directories(include)

# Source files
set(SOURCES
    src/mfp_base.cpp
    src/mfp_method1.cpp
    src/mfp_method2.cpp
    src/mfp_method3.cpp
)

# Create library
add_library(mfp STATIC ${SOURCES})
target_link_libraries(mfp ${GMP_LIBRARIES})

# Create test executable
add_executable(test_mfp test/test_mfp.cpp)
target_link_libraries(test_mfp mfp ${GMP_LIBRARIES})

# Add pthread for Method 3
find_package(Threads REQUIRED)
target_link_libraries(mfp Threads::Threads)
target_link_libraries(test_mfp Threads::Threads)
