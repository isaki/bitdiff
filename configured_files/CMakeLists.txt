# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright 2025 isaki

file(REAL_PATH "${CMAKE_CXX_COMPILER}" REAL_CXX_COMPILER)
cmake_path(GET REAL_CXX_COMPILER FILENAME REAL_CXX_COMPILER_NAME)

configure_file(
    "config.hpp.in"
    "${CMAKE_BINARY_DIR}/configured_files/include/bitdiff_internal/config.hpp"
    ESCAPE_QUOTES
)
