cmake_minimum_required(VERSION 3.10)
project(MFP VERSION 1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find GMP manually instead of using find_package
# This is more reliable across different systems
include(CheckIncludeFileCXX)
include(CheckLibraryExists)

# Check for GMP headers
check_include_file_cxx("gmp.h" HAVE_GMP_H)
if(NOT HAVE_GMP_H)
  message(FATAL_ERROR "GMP headers not found. Please install libgmp-dev package.")
endif()

# Check for GMP library
check_library_exists(gmp __gmpz_init "" HAVE_LIBGMP)
if(NOT HAVE_LIBGMP)
  message(FATAL_ERROR "GMP library not found. Please install libgmp-dev package.")
endif()

# Find Threads package
find_package(Threads REQUIRED)

# Include directories
include_directories(include)

# Source files
file(GLOB SOURCES "src/*.cpp")

# Create library
add_library(mfp_lib STATIC ${SOURCES})
target_link_libraries(mfp_lib gmp Threads::Threads)

# Create test executables
add_executable(test_mfp test/test_mfp.cpp)
target_link_libraries(test_mfp mfp_lib gmp Threads::Threads)

add_executable(test_prime test/test_prime.cpp)
target_link_libraries(test_prime mfp_lib gmp Threads::Threads)

add_executable(test_composite test/test_composite.cpp)
target_link_libraries(test_composite mfp_lib gmp Threads::Threads)

add_executable(test_concurrent test/test_concurrent.cpp)
target_link_libraries(test_concurrent mfp_lib gmp Threads::Threads)

# Add the CLI executable
add_executable(mfp_cli src/mfp_cli.cpp)
target_link_libraries(mfp_cli mfp_lib gmp Threads::Threads)

# Add the search space reduction test
add_executable(test_search_space_reduction test/test_search_space_reduction.cpp)
target_link_libraries(test_search_space_reduction mfp_lib gmp Threads::Threads)

# Installation
install(TARGETS mfp_lib DESTINATION lib)
install(DIRECTORY include/ DESTINATION include/mfp)
install(TARGETS mfp_cli DESTINATION bin)
