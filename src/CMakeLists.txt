# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright 2025-2026 isaki

# Include Boost
set(Boost_USE_STATIC_LIBS OFF)
find_package(Boost REQUIRED COMPONENTS program_options)
find_package(Threads REQUIRED)

add_executable(bitdiff
    reader.cpp
    dataout.cpp
    bitdiff.cpp
    version.cpp
    main.cpp
)

target_include_directories(bitdiff
    PRIVATE
    "${PROJECT_BINARY_DIR}/configured_files/include"
    "${PROJECT_SOURCE_DIR}/include"
)

target_link_libraries(bitdiff PRIVATE Threads::Threads Boost::program_options)
