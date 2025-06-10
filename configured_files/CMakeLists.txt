# Copyright 2025 isaki

configure_file(
    "config.hpp.in"
    "${CMAKE_BINARY_DIR}/configured_files/include/bitdiff_internal/config.hpp"
    ESCAPE_QUOTES
)
