/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright 2025-2026 isaki */

#include <ostream>
#include <stdexcept>
#include <algorithm>

// ReSharper disable once CppUnusedIncludeDirective
#include <cstdint>
// ReSharper disable once CppUnusedIncludeDirective
#include <cstddef>

#include <filesystem>
#include <string_view>

#include <memory>

#include "bitdiff/reader.hpp"
#include "bitdiff/dataout.hpp"
#include "bitdiff/bitdiff.hpp"

namespace bd = isaki::bitdiff;
namespace fs = std::filesystem;

namespace
{
    constexpr char OUT_DELIM = '\t';

    struct _ostream_state_cache_s final
    {
        std::ostream::iostate state;
        std::ostream::fmtflags flags;
        std::ostream * s;

        ~_ostream_state_cache_s()
        {
            if (s != nullptr)
            {
                s->exceptions(state);
                s->flags(flags);
            }
        }
    };

    template<bool Fast>
    void newline(std::ostream& os)
    {
        if constexpr (Fast)
        {
            os.write("\n", 1);
        }
        else
        {
            os << std::endl;
        }
    }
}

bd::BitDiff::BitDiff(std::string_view a, std::string_view b, std::size_t bufferSize, bool fastMode) :
    m_buffer_a(nullptr),
    m_buffer_b(nullptr),
    m_reader_a(nullptr),
    m_reader_b(nullptr),
    m_newline((fastMode) ? newline<true> : newline<false>),
    m_valid(true)
{
    // Temp values
    try
    {
        m_path_a.assign(a);
        m_path_b.assign(b);

        m_fsize_a = fs::file_size(m_path_a);
        m_fsize_b = fs::file_size(m_path_b);

        m_reader_a = new Reader(m_path_a, bufferSize);

        m_reader_b = new Reader(m_path_b, bufferSize);

        m_buffer_a = new unsigned char[bufferSize]();
        m_buffer_b = new unsigned char[bufferSize]();
    }
    catch (const std::exception& e)
    {
        std::cerr << "BitDiff initialization failure: " << e.what() << std::endl;

        // Cleanup will clear valid flag.
        cleanup();
        throw;
    }
}

bd::BitDiff::~BitDiff()
{
    cleanup();
}

std::uintmax_t bd::BitDiff::getFileASize() const noexcept
{
    return m_fsize_a;
}

std::uintmax_t bd::BitDiff::getFileBSize() const noexcept
{
    return m_fsize_b;
}

bd::diff_count bd::BitDiff::process(std::ostream& output, const bool printHeader, const DataOutType type)
{
    if (!m_valid)
    {
        throw std::runtime_error("Attempt to use invalid object");
    }

    // This will get automatically cleaned when it goes out of scope.
    const _ostream_state_cache_s outputCache = {
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
    std::uintmax_t bytesRead = 0;
    bd::diff_count ret = { .bytes = 0, .bits = 0 };

    if (printHeader)
    {
        output << "Offset\tByte in " << m_path_a << "\tByte in " << m_path_b;
        m_newline(output);
    }

    // Setup for output.
    std::unique_ptr<bd::DataOut> optr;
    switch (type)
    {
        case bd::DataOutType::Hex :
            optr = std::make_unique<bd::HexDataOut>(OUT_DELIM);
            break;

        case bd::DataOutType::Binary :
            optr = std::make_unique<bd::BinaryDataOut>(OUT_DELIM);
            break;

        default:
            optr = std::make_unique<bd::BitDataOut>(OUT_DELIM);
            break;
    }

    for (;;)
    {
        const std::size_t tmpA = m_reader_a->read(m_buffer_a);
        const std::size_t tmpB = m_reader_b->read(m_buffer_b);

        const std::size_t tmpX = std::min(tmpA, tmpB);

        for (std::size_t i = 0; i < tmpX; ++i)
        {
            if (const unsigned char a = m_buffer_a[i], b = m_buffer_b[i]; a != b)
            {
                optr->init(bytesRead + static_cast<std::uintmax_t>(i), a, b);

                // Counters
                ++ret.bytes;
                ret.bits += static_cast<std::uintmax_t>(optr->getDiffPopCount());

                // We could use the stream operator, but a raw write is faster.
                optr->print(output);
                m_newline(output);
            }
        }

        bytesRead += static_cast<std::uintmax_t>(tmpX);

        if (tmpA == 0 || tmpB == 0)
        {
            std::cerr << "End of one or both files reached" << std::endl;
            break;
        }

        if (tmpA != tmpB)
        {
            throw std::runtime_error("Read mismatch encountered before end of file reached");
        }
    }

    if (const std::uintmax_t expected = std::min(m_fsize_a, m_fsize_b); bytesRead != expected)
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
    if (m_reader_a != nullptr)
    {
        delete m_reader_a;
        m_reader_a = nullptr;
    }

    if (m_reader_b != nullptr)
    {
        delete m_reader_b;
        m_reader_b = nullptr;
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
