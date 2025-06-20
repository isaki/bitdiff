# SPDX-License-Identifier: Apache-2.0
# Copyright 2025 isaki

cmake_minimum_required(VERSION 4.0.2)

project(
    BitDiff
    VERSION 1.1.1
    LANGUAGES CXX
    HOMEPAGE_URL "https://github.com/isaki/bitdiff")

# Configure C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_CXX_EXTENSIONS Off)

# Global Compiler CMake Configuration
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Compiler flags
add_compile_options(-O2 -pipe -Wall -Wextra -Wpedantic -Werror -Wimplicit-fallthrough -Wformat)

# Compiler Specific Flags
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wtrampolines)
endif()

# Configure CMAKE
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# Add the directories
add_subdirectory(configured_files)
add_subdirectory(src)
