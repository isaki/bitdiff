/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2025 isaki */

#include <ostream>
#include <stdexcept>
#include <algorithm>

#include <cstdint>
#include <cstddef>
#include <filesystem>
#include <string_view>

#include <climits>
#include <memory>

#include "bitdiff/reader.hpp"
#include "bitdiff/dataout.hpp"
#include "bitdiff/bitdiff.hpp"

namespace bd = isaki::bitdiff;
namespace fs = std::filesystem;

namespace
{
    inline constexpr char OUT_DELIM = '\t';

    using DataOutFactory = std::unique_ptr<bd::DataOut> (*)(const unsigned char, const unsigned char, const char, char *);

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
}

bd::BitDiff::BitDiff(const std::string_view& a, const std::string_view& b, const size_t bufferSize) :
    m_bsize(bufferSize),
    m_valid(true),
    m_buffer_a(nullptr),
    m_buffer_b(nullptr),
    m_reader_a(nullptr),
    m_reader_b(nullptr)
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

bd::diff_count bd::BitDiff::process(std::ostream& output, const bool printHeader, const bd::DataOutType type)
{
    if (!m_valid)
    {
        throw std::runtime_error("Attempt to use invalid object");
    }

    // This will get automatically cleaned when it goes out of scope.
    const struct _ostream_state_cache_s outputCache = {
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

    // Setup for output.
    std::unique_ptr<char[]> outputBuffer;
    DataOutFactory factory;

    switch (type)
    {
        case bd::DataOutType::Hex :
            factory = [](const unsigned char a, const unsigned char b, const char delim, char * buffer)
            {
                auto ret = std::unique_ptr<bd::DataOut>(new bd::HexDataOut(a, b, delim, buffer));
                return ret;
            };

            outputBuffer = std::unique_ptr<char[]>(new char[UCHAR_HEX_COUNT + 1]);

            break;

        case bd::DataOutType::Binary :
            factory = [](const unsigned char a, const unsigned char b, const char delim, char * buffer)
            {
                auto ret = std::unique_ptr<bd::DataOut>(new bd::BinaryDataOut(a, b, delim, buffer));
                return ret;
            };

            outputBuffer = std::unique_ptr<char[]>(new char[UCHAR_BIT_COUNT + 1]);

            break;

        default:
            factory = [](const unsigned char a, const unsigned char b, const char delim, char * buffer)
            {
                auto ret = std::unique_ptr<bd::DataOut>(new bd::BitDataOut(a, b, delim, buffer));
                return ret;
            };

            outputBuffer = std::unique_ptr<char[]>(new char[UCHAR_BIT_COUNT + 1]);

            break;
    }

    for (;;)
    {
        const size_t tmpA = m_reader_a->read(m_buffer_a);
        const size_t tmpB = m_reader_b->read(m_buffer_b);

        const size_t tmpX = std::min(tmpA, tmpB);

        for (size_t i = 0; i < tmpX; ++i)
        {
            const unsigned char a = m_buffer_a[i];
            const unsigned char b = m_buffer_b[i];

            if (a != b)
            {
                auto optr = factory(a, b, OUT_DELIM, outputBuffer.get());

                // Counters
                ++ret.bytes;
                ret.bits += static_cast<uintmax_t>(optr->getDiffPopCount());

                // Output
                output << "0x" << std::setw(bd::UINTMAX_HEX_COUNT) << bytesRead + static_cast<uintmax_t>(i)
                    << OUT_DELIM << *(optr.get()) << std::endl;
            }
        }

        bytesRead += static_cast<uintmax_t>(tmpX);

        if (m_reader_a->eof() || m_reader_b->eof())
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
