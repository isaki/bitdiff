/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright 2025-2026 isaki */

#include <iostream>
#include <cstddef>
#include <charconv>
#include <stdexcept>
#include <system_error>
#include <string_view>

// We need to be able to move memory as required
#include <cstring>

// replace GCC built in popcount with C++ 20 version
#include <bit>

#include "bitdiff/dataout.hpp"

namespace bd = isaki::bitdiff;

namespace
{
    constexpr int HEX_RADIX = 16;
    constexpr int BIN_RADIX = 2;

    constexpr char NO_DIFF = '.';
    constexpr std::string_view HEX_PREFIX = "0x";
    constexpr std::string_view BIN_PREFIX = "0b";

    void to_bitwise_string(char* buffer, const std::size_t tokenSize, const unsigned char value, const unsigned char x)
    {
        for (std::size_t i = 0; i < tokenSize; ++i)
        {
            const std::size_t shift = tokenSize - i - 1;

            if (((x >> shift) & 1) == 0)
            {
                buffer[i] = NO_DIFF;
            }
            else if (((value >> shift) & 1) == 0)
            {
                buffer[i] = '0';
            }
            else
            {
                buffer[i] = '1';
            }
        }
    }

    void to_chars(char* start, char* end, const unsigned char value, const int radix)
    {
        auto result = std::to_chars(start, end, value, radix);

        if (result.ec != std::errc()) [[unlikely]]
        {
            throw std::runtime_error(std::make_error_code(result.ec).message());
        }
        else if (result.ptr != end)
        {
            // We didn't fill. We can rely on filling the stream and have to
            // manage the ostream flags, or, we can shift the data and pad in
            // our own zeros (the character, not the number);

             // How many characters we need to fill.
            const ptrdiff_t offset = end - result.ptr;

            // The full size of the buffer we need to fill.
            const ptrdiff_t full = end - start;

            // The number of characters we need to move.
            const ptrdiff_t difference = full - offset;

            std::memmove(start + offset, start, difference);
            std::memset(start, '0', offset);
        }
    }
}

//
// BASE CLASS
//

bd::DataOut::~DataOut()
{
    delete[] m_buffer;
}

bd::DataOut::DataOut(std::string_view prefix, std::size_t tokenSize, char delim) :
    m_tokenSize(tokenSize),
    m_buffer(nullptr),
    m_posA(nullptr),
    m_posB(nullptr),
    m_a(0),
    m_b(0)
{
    const std::size_t prefixLen = prefix.size();

    m_buffer = new char[((tokenSize + prefixLen) * 2) + 2](); // calloc/memset zero
    m_posA = m_buffer + prefixLen;
    m_posB = m_buffer + (2 * prefixLen) + tokenSize + 1;

    m_buffer[tokenSize + prefixLen] = delim;

    std::memcpy(m_buffer, prefix.data(), prefixLen);
    std::memcpy(m_buffer + prefixLen + tokenSize + 1, prefix.data(), prefixLen);
}

int bd::DataOut::getDiffPopCount() const
{
    return std::popcount<unsigned char>(m_a ^ m_b);
}

void bd::DataOut::init(unsigned char dataA, unsigned char dataB) noexcept
{
    m_a = dataA;
    m_b = dataB;
}

//
// HEX
//

bd::HexDataOut::~HexDataOut() = default;

bd::HexDataOut::HexDataOut(char delim) :
    super(HEX_PREFIX, bd::UCHAR_HEX_COUNT, delim) {}

void bd::HexDataOut::print(std::ostream& os) const
{
    printBuffer(os, [](char* buff, std::size_t len, unsigned char value)
    {
        to_chars(buff, buff + len, value, HEX_RADIX);
    }
    );
}

//
// BINARY
//

bd::BinaryDataOut::~BinaryDataOut() = default;

bd::BinaryDataOut::BinaryDataOut(char delim) :
    super(BIN_PREFIX, bd::UCHAR_BIT_COUNT, delim) {}

void bd::BinaryDataOut::print(std::ostream& os) const
{
    printBuffer(os, [](char* buff, std::size_t len, unsigned char value)
    {
        to_chars(buff, buff + len, value, BIN_RADIX);
    }
    );
}

//
// BITWISE
//

bd::BitDataOut::~BitDataOut() = default;

bd::BitDataOut::BitDataOut(char delim) :
    super(BIN_PREFIX, bd::UCHAR_BIT_COUNT, delim),
    m_xor(0) {}

int bd::BitDataOut::getDiffPopCount() const
{
    return std::popcount<unsigned char>(m_xor);
}

void bd::BitDataOut::init(unsigned char dataA, unsigned char dataB) noexcept
{
    super::init(dataA, dataB);
    m_xor = dataA ^ dataB;
}

void bd::BitDataOut::print(std::ostream& os) const
{
    printBuffer(os, [x = m_xor](char* buff, std::size_t len, unsigned char value)
    {
        to_bitwise_string(buff, len, value, x);
    }
    );
}
