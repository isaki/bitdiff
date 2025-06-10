# Copyright 2024 isaki

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost REQUIRED COMPONENTS program_options)

add_executable(bitdiff
    bitdiff.cpp

    # Main should be last
    main.cpp
)

target_include_directories(bitdiff
    PRIVATE
    "${CMAKE_BINARY_DIR}/configured_files/include"
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_BINARY_DIR}/include"
)

target_link_libraries(bitdiff PRIVATE Boost::program_options)
