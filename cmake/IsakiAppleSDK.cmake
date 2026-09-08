# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright 2026 isaki

#
# Initialize (Must be called before project)
#
function(isaki_initialize_apple)

    if (CMAKE_PROJECT_NAME)
        # If this was a submodule, we are past project and do not want to
        # do this to avoid destroying any parent toolchain configuration
        return()
    endif()

    # Query the underlying Apple SDK version layer
    execute_process(
        COMMAND xcrun --sdk macosx --show-sdk-version
        OUTPUT_VARIABLE MACOS_SDK_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    
    # Propagate the SDK version up to parent scope so Phase 2 can read it
    set(MACOS_SDK_VERSION "${MACOS_SDK_VERSION}" PARENT_SCOPE)

    # Strict validation check for legacy SDKs (less than macOS 13 Sequoia baseline)
    if(MACOS_SDK_VERSION VERSION_LESS "13.0")
        message(STATUS "Legacy SDK detected (${MACOS_SDK_VERSION}). Injecting initialization hooks...")

        # Determine the compiler binary using your fallback logic rules
        set(LOCAL_CXX_COMPILER "${CMAKE_CXX_COMPILER}")

        # Check the env in case CMAKE hasnt set this yet
        if(NOT LOCAL_CXX_COMPILER)
            set(LOCAL_CXX_COMPILER "$ENV{CXX}")
        endif()

        # Guess a compiler, we choose MacPorts clang
        if(NOT LOCAL_CXX_COMPILER)
            set(LOCAL_CXX_COMPILER "/opt/local/bin/clang")
        endif()

        # Execute a manual compiler check to extract the major version before project() runs
        execute_process(
            COMMAND ${LOCAL_CXX_COMPILER} -dumpversion
            OUTPUT_VARIABLE MP_CLANG_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )

        string(REGEX MATCH "^[0-9]+" CLANG_MAJOR_VERSION "${MP_CLANG_VERSION}")

        if(NOT CLANG_MAJOR_VERSION)
            message(FATAL_ERROR "Build Aborted: Unable to determine the local compiler version via ${LOCAL_CXX_COMPILER}")
        endif()

        # Overwrite the dynamic modular scanner variable before cmake locks it
        set(SCANNER_PATH "/opt/local/libexec/llvm-${CLANG_MAJOR_VERSION}/bin/clang-scan-deps")
        set(CMAKE_CXX_COMPILER_CLANG_SCAN_DEPS "${SCANNER_PATH}" CACHE FILEPATH "MacPorts modules scanner override" FORCE)
        
        message(STATUS "Dynamic module scanner registered: ${SCANNER_PATH}")

    endif()

    # Mark initialization as successfully completed in the cache
    set(ISAKI_APPLE_SDK_INITIALIZED TRUE CACHE INTERNAL "Tracks whether initialization pass executed")
endfunction()

#
# Configure the target
#
function(isaki_configure_apple TARGET_NAME)

    if (NOT ISAKI_APPLE_SDK_INITIALIZED)
        # init wasnt called, skip this.
        return()
    endif()

    # Safeguard validation rule for modern machines
    if(MACOS_SDK_VERSION VERSION_LESS "13.0")
        message(STATUS "Executing local architecture isolation configurations...")

        # Enforce that if they are on a legacy SDK, they MUST use Upstream Clang 21+ (MacPorts)
        if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_VERSION VERSION_LESS "21.0")
            message(FATAL_ERROR "Build Aborted: C++20 builds on legacy macOS require Upstream Clang 21.0 or newer (MacPorts). Found: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
        endif()

        # Re-verify our dynamic version tracking now that CMake's properties are live
        string(REGEX MATCH "^[0-9]+" CLANG_MAJOR_VERSION "${CMAKE_CXX_COMPILER_VERSION}")
        
        set(MP_LLVM_INCLUDE_DIR "/opt/local/libexec/llvm-${CLANG_MAJOR_VERSION}/include/c++/v1")
        set(MP_LLVM_LIB_DIR "/opt/local/libexec/llvm-${CLANG_MAJOR_VERSION}/lib")
        
        message(STATUS "Dynamically mapped runtime flag assets:")
        message(STATUS " -> Include: ${MP_LLVM_INCLUDE_DIR}")
        message(STATUS " -> Library: ${MP_LLVM_LIB_DIR}")

        # Stub out the broken implicit thread compilation verification checks
        set(CMAKE_HAVE_THREADS_LIBRARY 1 CACHE INTERNAL "")
        set(Threads_FOUND TRUE CACHE INTERNAL "")
        set(CMAKE_THREAD_LIBS_INIT "-lpthread" CACHE INTERNAL "")

        # Extract the legacy sysroot
        execute_process(
            COMMAND xcrun --sdk macosx --show-sdk-path
            OUTPUT_VARIABLE MAC_SYSROOT_PATH
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        # Set compiler options
        target_compile_options(
            "${TARGET_NAME}"
            PRIVATE
            -stdlib=libc++
            -nostdinc++ -I${MP_LLVM_INCLUDE_DIR}
            -isysroot ${MAC_SYSROOT_PATH}
        )

        # Configure the Linker
        set(CMAKE_BUILD_WITH_INSTALL_RPATH OFF PARENT_SCOPE)
        set(CMAKE_INSTALL_RPATH_USE_LINK_PATH ON PARENT_SCOPE)
        
        target_link_options(
            "${TARGET_NAME}"
            PRIVATE
            -stdlib=libc++
            -L${MP_LLVM_LIB_DIR}
            -fuse-ld=/opt/local/libexec/llvm-${CLANG_MAJOR_VERSION}/bin/ld64.lld
        )
    endif()
endfunction()

