/* Copyright 2025 isaki */

#ifndef __BITDIFF_HPP__
#define __BITDIFF_HPP__

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <ostream>
#include <fstream>
#include <filesystem>

namespace isaki::bitdiff
{
    class BitDiff
    {
        public:
            BitDiff(const std::string_view& a, const std::string_view& b, const size_t bufferSize);
            ~BitDiff();

            // Returns the number of differences.
            uintmax_t process(std::ostream& output);
        private:
            BitDiff() = delete;
            BitDiff(const BitDiff&) = delete;
            BitDiff & operator=(const BitDiff&) = delete;

            void cleanup() noexcept;

            const size_t m_bsize;
            uintmax_t m_fsize_a;
            uintmax_t m_fsize_b;
            bool m_valid;

            std::filesystem::path m_path_a;
            std::filesystem::path m_path_b;

            char * m_buffer_a;
            char * m_buffer_b;
            std::ifstream * m_is_a;
            std::ifstream * m_is_b;
    };
            
}

#endif
