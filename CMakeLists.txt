cmake_minimum_required(VERSION 3.20)
project(GameEngine VERSION 1.0.0 LANGUAGES C CXX)

# C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(ENGINE_BUILD_SANDBOX "Build sandbox application" ON)
option(ENGINE_BUILD_TESTS "Build unit tests" OFF)
option(ENGINE_ENABLE_PROFILING "Enable profiling" OFF)

# Output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Include cmake modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# External dependencies
add_subdirectory(external)

# Engine library
add_subdirectory(engine)

# Sandbox application
if(ENGINE_BUILD_SANDBOX)
    add_subdirectory(sandbox)
endif()

# Tests
if(ENGINE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
