# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright 2026 isaki

function(isaki_strip TARGET_NAME)
    # Ensure the target actually exists
    if (NOT TARGET ${TARGET_NAME})
        message(FATAL_ERROR "isaki_strip: '${TARGET_NAME}' is not a valid CMake target.")
    endif()

    # Only strip in Release (or unspecified) builds
    if (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "Release")

        if (APPLE)
            add_custom_command(
                TARGET ${TARGET_NAME} POST_BUILD
                COMMAND ${CMAKE_STRIP} -u -r $<TARGET_FILE:${TARGET_NAME}>
                COMMENT "Stripping macOS executable ${TARGET_NAME} with ${CMAKE_STRIP} -u -r"
            )
        elseif (UNIX)
            add_custom_command(
                TARGET ${TARGET_NAME} POST_BUILD
                COMMAND ${CMAKE_STRIP} --strip-unneeded $<TARGET_FILE:${TARGET_NAME}>
                COMMENT "Stripping Linux executable ${TARGET_NAME} with ${CMAKE_STRIP} --strip-unneeded"
            )
        endif()

    endif()
endfunction()
