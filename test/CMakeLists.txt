cmake_minimum_required(VERSION 3.12)

# Add Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/release-1.11.0.zip
)
FetchContent_MakeAvailable(googletest)

# Test executables
add_executable(system_test system_test.cpp)
target_link_libraries(system_test mfp_enhanced gtest gtest_main)

add_executable(unit_tests 
    unit/cpu_detector_test.cpp
    unit/memory_storage_detector_test.cpp
    unit/gpu_detector_test.cpp
    unit/resource_manager_test.cpp
    unit/configuration_manager_test.cpp
    unit/mfp_methods_test.cpp
)
target_link_libraries(unit_tests mfp_enhanced gtest gtest_main)

# Add tests
add_test(NAME SystemTest COMMAND system_test)
add_test(NAME UnitTests COMMAND unit_tests)

# Create test directory structure
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/test_data)
