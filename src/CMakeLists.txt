# SPDX-License-Identifier: Apache-2.0
# Copyright 2025 isaki

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost REQUIRED COMPONENTS program_options)

add_executable(bitdiff
    reader.cpp
    dataout.cpp
    bitdiff.cpp
    version.cpp
    main.cpp
)

target_include_directories(bitdiff
    PRIVATE
    "${CMAKE_BINARY_DIR}/configured_files/include"
    "${PROJECT_SOURCE_DIR}/include"
)

target_link_libraries(bitdiff PRIVATE Boost::program_options)
