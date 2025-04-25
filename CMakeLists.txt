cmake_minimum_required(VERSION 3.10)
project(mfp_implementation VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find GMP package
find_library(GMP_LIBRARIES NAMES gmp)
find_path(GMP_INCLUDE_DIRS NAMES gmp.h)

if(NOT GMP_LIBRARIES OR NOT GMP_INCLUDE_DIRS)
    message(FATAL_ERROR "GMP library not found. Please install libgmp-dev")
endif()

# Find Threads package
find_package(Threads REQUIRED)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${GMP_INCLUDE_DIRS}
)

# Source files
set(SOURCES
    src/mfp_base.cpp
    src/mfp_method1.cpp
    src/mfp_method2.cpp
    src/mfp_method3.cpp
    src/mfp_system.cpp
    src/main.cpp
)

# Create executable
add_executable(mfp_app ${SOURCES})

# Link libraries
target_link_libraries(mfp_app
    ${GMP_LIBRARIES}
    Threads::Threads
)

# Install target
install(TARGETS mfp_app
    RUNTIME DESTINATION bin
)
