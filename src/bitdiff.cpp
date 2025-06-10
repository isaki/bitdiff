/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2025 isaki */

#include <iostream>
#include <stdexcept>
#include <cstring>
#include <algorithm>

#include <cstdint>
#include <cstddef>
#include <fstream>
#include <filesystem>
#include <string_view>

#include <climits>

#include "bitdiff/bitdiff.hpp"

namespace bd = isaki::bitdiff;

namespace
{
    inline constexpr size_t OFFSET_WIDTH = sizeof(uintmax_t) * (CHAR_BIT >> 2);
    inline constexpr size_t BYTE_WIDTH = sizeof(char) * (CHAR_BIT >> 2);

    struct _of_state_cache_s
    {
        std::ostream::iostate state;
        std::ostream::fmtflags flags;
        std::ostream * s;

        ~_of_state_cache_s()
        {
            if (s != nullptr)
            {
                s->exceptions(state);
                s->flags(flags);
            }
        }
    };

    inline void _closeStream(std::ifstream * s)
    {
        if (s->is_open())
        {
            s->close();
        }
    }

    // This function is specific to GCC/Clang compilers
    inline int _popcount(unsigned char c) {
#if defined(__GNUC__)
        return __builtin_popcount(c);
#else
        #error "GNU Extensions not supported"
#endif
    }

    size_t _fillBuffer(std::ifstream * in, unsigned char * buffer, const size_t len)
    {
        size_t read = 0;

        char * sbuff = reinterpret_cast<char *>(buffer);

        while (read < len && !in->eof())
        {
            in->read(sbuff, len - read);
            read += in->gcount();
        }

        return read;
    }
}

bd::BitDiff::BitDiff(const std::string_view& a, const std::string_view& b, const size_t bufferSize) :
    m_bsize(bufferSize),
    m_valid(true),
    m_buffer_a(nullptr),
    m_buffer_b(nullptr),
    m_is_a(nullptr),
    m_is_b(nullptr)
{
    // Temp values
    try
    {
        m_path_a.assign(a);
        m_path_b.assign(b);

        m_fsize_a = std::filesystem::file_size(m_path_a);
        m_fsize_b = std::filesystem::file_size(m_path_b);

        m_is_a = new std::ifstream();
        m_is_a->open(m_path_a, std::ios_base::binary | std::ios_base::in);
        if (!m_is_a->is_open())
        {
            std::string err;
            err.append("Unable to open ");
            err.append(a);
            throw std::runtime_error(err);
        }
        
        m_is_a->exceptions(std::ifstream::badbit);

        m_is_b = new std::ifstream();
        m_is_b->open(m_path_b, std::ios_base::binary | std::ios_base::in);
        if (!m_is_b->is_open())
        {
            std::string err;
            err.append("Unable to open ");
            err.append(b);
            throw std::runtime_error(err);
        }
        
        m_is_b->exceptions(std::ifstream::badbit);

        m_buffer_a = new unsigned char[bufferSize];
        memset(m_buffer_a, 0, bufferSize * sizeof(char));

        m_buffer_b = new unsigned char[bufferSize];
        memset(m_buffer_b, 0, bufferSize * sizeof(char));
    }
    catch (const std::exception& e)
    {
        std::cerr << "BitDiff initialization failure: " << e.what() << std::endl;

        // Cleanup will clear valid flag.
        cleanup();
        throw e;
    }
}

bd::BitDiff::~BitDiff()
{
    cleanup();
}

uintmax_t bd::BitDiff::getFileASize() const noexcept
{
    return m_fsize_a;
}

uintmax_t bd::BitDiff::getFileBSize() const noexcept
{
    return m_fsize_b;
}

bd::diff_count bd::BitDiff::process(std::ostream& output, const bool printHeader)
{
    if (!m_valid)
    {
        throw std::runtime_error("Attempt to use invalid object");
    }

    // This will get automatically cleaned when it goes out of scope.
    const struct _of_state_cache_s outputCache = {
        .state = output.exceptions(),
        .flags = output.flags(),
        .s = &output
    };

    output.exceptions(std::ostream::failbit | std::ostream::badbit);
    output << std::hex << std::setfill('0');

    m_valid = false;

    if (m_fsize_a != m_fsize_b)
    {
        std::cerr
            << m_path_a << " (" << m_fsize_a << ")"
            << " and "
            << m_path_b << " (" << m_fsize_b << ")"
            << " differ in size; diff will end at smaller size"
            << std::endl;
    }

    // First, we need to read from each buffer.    
    uintmax_t bytesRead = 0;
    bd::diff_count ret = { .bytes = 0, .bits = 0 };

    if (printHeader)
    {
        output << "Offset\tByte in " << m_path_a << "\tByte in " << m_path_b << std::endl;
    }

    for (;;)
    {
        const size_t tmpA = _fillBuffer(m_is_a, m_buffer_a, m_bsize);
        const size_t tmpB = _fillBuffer(m_is_b, m_buffer_b, m_bsize);

        const size_t tmpX = std::min(tmpA, tmpB);

        for (size_t i = 0; i < tmpX; ++i)
        {
            const unsigned char a = m_buffer_a[i];
            const unsigned char b = m_buffer_b[i];

            if (a != b)
            {
                const int pcount = _popcount(a ^ b);

                ++ret.bytes;
                ret.bits += static_cast<uintmax_t>(pcount);

                output << "0x" << std::setw(OFFSET_WIDTH) << bytesRead + static_cast<uintmax_t>(i)
                    << "\t0x" << std::setw(BYTE_WIDTH) << static_cast<unsigned int>(a)
                    << "\t0x" << std::setw(BYTE_WIDTH) << static_cast<unsigned int>(b)
                    << std::endl;
            }
        }

        bytesRead += static_cast<uintmax_t>(tmpX);

        if (m_is_a->eof() || m_is_b->eof())
        {
            std::cerr << "End of one or both files reached" << std::endl;
            break;
        }

        if (tmpA != tmpB)
        {
            throw std::runtime_error("Read mismatch encountered before end of file reached");
        }
    }

    const uintmax_t expected = std::min(m_fsize_a, m_fsize_b);
    if (bytesRead != expected)
    {
        std::string err;
        err.append("Bytes read ");
        err.append(std::to_string(bytesRead));
        err.append(" not equal to expected ");
        err.append(std::to_string(expected));

        throw std::runtime_error(err);
    }

    return ret;
}

void bd::BitDiff::cleanup() noexcept
{
    if (m_is_a != nullptr)
    {
        try
        {
            _closeStream(m_is_a);
        }
        catch (const std::exception& e)
        {
            std::cerr << "Failed to close stream A: " << e.what() << std::endl;
        }

        delete m_is_a;
        m_is_a = nullptr;
    }

    if (m_is_b != nullptr)
    {
        try
        {
            _closeStream(m_is_b);
        }
        catch (const std::exception& e)
        {
            std::cerr << "Failed to close stream B: " << e.what() << std::endl;
        }

        delete m_is_b;
        m_is_b = nullptr;
    }
    
    if (m_buffer_a != nullptr)
    {
        delete[] m_buffer_a;
        m_buffer_a = nullptr;
    }

    if (m_buffer_b != nullptr)
    {
        delete[] m_buffer_b;
        m_buffer_b = nullptr;
    }
    
    m_valid = false;
}
