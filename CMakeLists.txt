cmake_minimum_required(VERSION 3.12)
project(mfp_enhanced VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(BUILD_TESTS "Build test programs" ON)
option(ENABLE_CUDA "Enable CUDA support" ON)
option(ENABLE_METAL "Enable Metal support" ON)

# Find required packages
find_package(GMP REQUIRED)
find_package(Threads REQUIRED)

# Check for CUDA
if(ENABLE_CUDA)
    include(CheckLanguage)
    check_language(CUDA)
    if(CMAKE_CUDA_COMPILER)
        enable_language(CUDA)
        set(CUDA_FOUND TRUE)
        add_definitions(-DHAS_CUDA)
        message(STATUS "CUDA support enabled")
    else()
        set(CUDA_FOUND FALSE)
        message(STATUS "CUDA compiler not found, CUDA support disabled")
    endif()
endif()

# Check for Metal (macOS only)
if(ENABLE_METAL AND APPLE)
    set(METAL_FOUND TRUE)
    add_definitions(-DHAS_METAL)
    message(STATUS "Metal support enabled")
else()
    set(METAL_FOUND FALSE)
    if(ENABLE_METAL)
        message(STATUS "Metal is only available on macOS, Metal support disabled")
    endif()
endif()

# Configure header file
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mfp_config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/mfp_config.h
)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/include
    ${GMP_INCLUDE_DIRS}
)

# Source files
set(MFP_SOURCES
    src/mfp_base.cpp
    src/mfp_method1.cpp
    src/mfp_method2.cpp
    src/mfp_method3.cpp
    src/cpu_detector.cpp
    src/memory_storage_detector.cpp
    src/gpu_detector.cpp
    src/resource_manager.cpp
    src/configuration_manager.cpp
    src/mfp_system.cpp
    src/prime_database.cpp
)

# Add CUDA sources if enabled
if(CUDA_FOUND)
    list(APPEND MFP_SOURCES src/cuda_accelerator.cpp)
    
    # CUDA specific files
    set(CUDA_SOURCES
        src/cuda/mfp_kernels.cu
        src/cuda/cuda_utils.cu
    )
    
    # Set CUDA flags
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --expt-extended-lambda -arch=sm_50")
endif()

# Add Metal sources if enabled
if(METAL_FOUND)
    list(APPEND MFP_SOURCES src/metal_accelerator.mm)
    
    # Metal specific files
    set(METAL_SOURCES
        src/metal/metal_utils.mm
    )
    
    # Set Metal flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -framework Metal -framework Foundation")
endif()

# Create library
add_library(mfp_enhanced ${MFP_SOURCES})

# Add CUDA library if enabled
if(CUDA_FOUND)
    cuda_add_library(mfp_cuda ${CUDA_SOURCES})
    target_link_libraries(mfp_enhanced mfp_cuda)
endif()

# Add Metal library if enabled
if(METAL_FOUND)
    add_library(mfp_metal ${METAL_SOURCES})
    target_link_libraries(mfp_enhanced mfp_metal)
endif()

# Link libraries
target_link_libraries(mfp_enhanced
    ${GMP_LIBRARIES}
    Threads::Threads
)

# Create executable
add_executable(mfp_app src/main.cpp)
target_link_libraries(mfp_app mfp_enhanced)

# Install targets
install(TARGETS mfp_enhanced mfp_app
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Install headers
install(DIRECTORY include/
    DESTINATION include/mfp_enhanced
    FILES_MATCHING PATTERN "*.h"
)

# Install configuration header
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/mfp_config.h
    DESTINATION include/mfp_enhanced
)

# Build tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

# Add documentation
add_subdirectory(docs)
