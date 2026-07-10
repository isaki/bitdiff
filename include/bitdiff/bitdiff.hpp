/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright 2025-2026 isaki */

#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <ostream>
#include <filesystem>

#include "bitdiff/reader.hpp"

namespace isaki::bitdiff
{
    struct diff_count
    {
        std::uintmax_t bytes;
        std::uintmax_t bits;
    };

    class BitDiff final
    {
    public:
        BitDiff() = delete;
        BitDiff(const BitDiff&) = delete;
        BitDiff& operator=(const BitDiff&) = delete;
        BitDiff(BitDiff&&) = delete;
        BitDiff& operator=(BitDiff&&) = delete;

        BitDiff(std::string_view a, std::string_view b, std::size_t bufferSize, bool fastMode);
        ~BitDiff();

        // Returns the number of differences.
        [[nodiscard]] diff_count process(std::ostream& output, bool printHeader, DataOutType type);

        [[nodiscard]] std::uintmax_t getFileASize() const noexcept;
        [[nodiscard]] std::uintmax_t getFileBSize() const noexcept;

    private:
        using NewlineFunc = void (*)(std::ostream&);

        void cleanup() noexcept;

        std::uintmax_t m_fsize_a;
        std::uintmax_t m_fsize_b;

        std::filesystem::path m_path_a;
        std::filesystem::path m_path_b;

        unsigned char* m_buffer_a;
        unsigned char* m_buffer_b;

        Reader* m_reader_a;
        Reader* m_reader_b;

        NewlineFunc m_newline;
        bool m_valid;
    };
}
