/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2025 isaki */

#include <iostream>
#include <cstddef>
#include <charconv>
#include <stdexcept>
#include <system_error>
#include <string_view>

// We need to be able to move memory as required
#include <cstring>

#include "bitdiff/dataout.hpp"

namespace bd = isaki::bitdiff;

namespace
{
    inline constexpr int HEX_RADIX = 16;
    inline constexpr int BIN_RADIX = 2;

    inline constexpr char NO_DIFF = '.';
    inline constexpr std::string_view HEX_PREFIX = "0x";
    inline constexpr std::string_view BIN_PREFIX = "0b";

    void _to_bitwise_string(const unsigned char value, const unsigned char x, char * buffer, const size_t bufferSize)
    {
        for (size_t i = 0; i < bufferSize; ++i)
        {
            const size_t shift = bufferSize - i - 1;

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

    void _to_chars(char * start, char * end, const unsigned char value, const int radix)
    {
        auto result = std::to_chars(start, end, value, radix);

        if (result.ec != std::errc())
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

            memmove(start + offset, start, difference);
            memset(start, '0', offset);
        }
    }

    // This function is specific to GCC/Clang compilers
    inline int _popcount(unsigned char c) {
#if defined(__GNUC__)
        return __builtin_popcount(static_cast<unsigned int>(c));
#else
#error "GNU Extensions not supported"
#endif
    }
}

//
// OPERATORS
//

std::ostream& bd::operator<<(std::ostream& os, const bd::DataOut& obj)
{
    obj.print(os);
    return os;
}

//
// BASE CLASS
//

bd::DataOut::~DataOut()
{
    delete[] m_buffer;
}

bd::DataOut::DataOut(const char delim, const size_t bufferSize) :
    m_delim(delim),
    m_a(0),
    m_b(0),
    m_buffer(nullptr)
{
    m_buffer = new char[bufferSize];
}

int bd::DataOut::getDiffPopCount() const
{
    return _popcount(m_a ^ m_b);
}

void bd::DataOut::init(const unsigned char dataA, const unsigned char dataB) noexcept
{
    m_a = dataA;
    m_b = dataB;
}

//
// HEX
//

bd::HexDataOut::~HexDataOut() = default;

bd::HexDataOut::HexDataOut(const char delim) :
    super(delim, bd::UCHAR_HEX_COUNT + 1) {}

void bd::HexDataOut::print(std::ostream& os) const
{
    // Ensure we end up null terminated for all operations.
    m_buffer[UCHAR_HEX_COUNT] = '\0';

    _to_chars(m_buffer, m_buffer + UCHAR_HEX_COUNT, m_a, HEX_RADIX);
    os << HEX_PREFIX << m_buffer << m_delim;

    _to_chars(m_buffer, m_buffer + UCHAR_HEX_COUNT, m_b, HEX_RADIX);
    os << HEX_PREFIX << m_buffer;
}

//
// BINARY
//

bd::BinaryDataOut::~BinaryDataOut() = default;

bd::BinaryDataOut::BinaryDataOut(const char delim) :
    super(delim, bd::UCHAR_BIT_COUNT + 1) {}

void bd::BinaryDataOut::print(std::ostream& os) const
{
    // Ensure we end up null terminated for all operations.
    m_buffer[UCHAR_BIT_COUNT] = '\0';

    _to_chars(m_buffer, m_buffer + UCHAR_BIT_COUNT, m_a, BIN_RADIX);
    os << BIN_PREFIX << m_buffer << m_delim;

    _to_chars(m_buffer, m_buffer + UCHAR_BIT_COUNT, m_b, BIN_RADIX);
    os << BIN_PREFIX << m_buffer;
}

//
// BITWISE
//

bd::BitDataOut::~BitDataOut() = default;

bd::BitDataOut::BitDataOut(const char delim) :
    super(delim, bd::UCHAR_BIT_COUNT + 1),
    m_xor(0) {}

int bd::BitDataOut::getDiffPopCount() const
{
    return _popcount(m_xor);
}

void bd::BitDataOut::init(const unsigned char dataA, const unsigned char dataB) noexcept
{
    super::init(dataA, dataB);
    m_xor = dataA ^ dataB;
}

void bd::BitDataOut::print(std::ostream& os) const
{
    // Ensure we end up null terminated for all operations.
    m_buffer[UCHAR_BIT_COUNT] = '\0';

    _to_bitwise_string(m_a, m_xor, m_buffer, UCHAR_BIT_COUNT);
    os << BIN_PREFIX << m_buffer << m_delim;

    _to_bitwise_string(m_b, m_xor, m_buffer, UCHAR_BIT_COUNT);
    os << BIN_PREFIX << m_buffer;
}
