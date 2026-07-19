# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright 2025-2026 isaki

cmake_minimum_required(VERSION 4.0.3)

project(
    BitDiff
    VERSION 1.4.6
    LANGUAGES CXX
    HOMEPAGE_URL "https://github.com/isaki/bitdiff")

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# Configure C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_CXX_EXTENSIONS Off)

# Global Compiler CMake Configuration
set(CMAKE_POSITION_INDEPENDENT_CODE On)

# Compiler flags
add_compile_options(-O3 -march=native -pipe -Wall -Wextra -Wpedantic -Werror -Wimplicit-fallthrough -Wformat)

# Compiler Specific Flags
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wtrampolines)
endif()

# Configure CMAKE
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_CURRENT_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_CURRENT_BINARY_DIR}/lib")

# Add the directories
add_subdirectory(configured_files)
add_subdirectory(src)
