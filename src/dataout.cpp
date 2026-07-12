/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright 2025-2026 isaki */

#include <iostream>
#include <cstddef>
#include <cstdint>
#include <charconv>
#include <stdexcept>
#include <system_error>
#include <string_view>
#include <cassert>

// We need to be able to move memory as required
#include <cstring>

// replace GCC built in popcount with C++ 20 version
#include <bit>

// Compile time safety
#include <type_traits>

#include "bitdiff/dataout.hpp"

namespace bd = isaki::bitdiff;

namespace
{
    constexpr int HEX_RADIX = 16;
    constexpr int BIN_RADIX = 2;

    constexpr char NO_DIFF = '.';
    constexpr std::string_view HEX_PREFIX = "0x";
    constexpr std::string_view BIN_PREFIX = "0b";

    constexpr std::size_t UCHAR_HEX_COUNT = sizeof(unsigned char) * (CHAR_BIT >> 2);
    constexpr std::size_t UCHAR_BIT_COUNT = sizeof(unsigned char) * CHAR_BIT;
    constexpr std::size_t UINTMAX_HEX_COUNT = sizeof(std::uintmax_t) * (CHAR_BIT >> 2);

    void to_bitwise_string(char* buffer, const std::size_t tokenSize, const unsigned char value, const unsigned char x) noexcept
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

    template<typename T>
    requires std::is_integral_v<T> && std::is_unsigned_v<T>
    void to_chars(char* start, char* end, const T value, const int radix) noexcept
    {
        auto result = std::to_chars(start, end, value, radix);

        assert(result.ec == std::errc());

        if (result.ptr != end)
        {
            // We didn't fill. We can rely on filling the stream and have to
            // manage the ostream flags, or, we can shift the data and pad in
            // our own zeros (the character, not the number);

             // How many characters we need to fill.
            const std::ptrdiff_t offset = end - result.ptr;

            // The full size of the buffer we need to fill.
            const std::ptrdiff_t full = end - start;

            // The number of characters we need to move.
            const std::ptrdiff_t difference = full - offset;

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
    m_recordSize(HEX_PREFIX.size() + UINTMAX_HEX_COUNT
        + 1
        + prefix.size() + tokenSize
        + 1
        + prefix.size() + tokenSize),
    m_buffer(nullptr),
    m_posAddr(nullptr),
    m_posA(nullptr),
    m_posB(nullptr),
    m_a(0),
    m_b(0)
{
    //
    // --- Allocate scratch space --- //
    //

    m_buffer = new char[m_recordSize]; // memset no longer needed, raw buffer, not a string

    //
    // --- Cache mutable scratch regions of memory --- //
    //

    m_posAddr = m_buffer + HEX_PREFIX.size();

    // A starts address and 1 delim, and one token prefix after m_posAddr.
    m_posA = m_posAddr + UINTMAX_HEX_COUNT + prefix.size() + 1;

    // B is 1 token, 1 delim, and one token prefix past A.
    m_posB = m_posA + prefix.size() + tokenSize + 1;

    //
    // --- Populate static regions of scratch memory --- //
    //

    // Address prefix
    std::memcpy(m_buffer, HEX_PREFIX.data(), HEX_PREFIX.size());

    // delim after address
    m_posAddr[UINTMAX_HEX_COUNT] = delim;

    // First token prefix
    std::memcpy(m_posA - prefix.size(), prefix.data(), prefix.size());

    // delim after first token
    m_posA[tokenSize] = delim;

    // Second token prefix.
    std::memcpy(m_posB - prefix.size(), prefix.data(), prefix.size());
}

int bd::DataOut::getDiffPopCount() const
{
    return std::popcount<unsigned char>(m_a ^ m_b);
}

void bd::DataOut::init(std::uintmax_t address, unsigned char dataA, unsigned char dataB) noexcept
{
    to_chars<std::uintmax_t>(m_posAddr, m_posAddr + UINTMAX_HEX_COUNT, address, HEX_RADIX);
    m_a = dataA;
    m_b = dataB;
}

//
// HEX
//

bd::HexDataOut::~HexDataOut() = default;

bd::HexDataOut::HexDataOut(char delim) :
    super(HEX_PREFIX, UCHAR_HEX_COUNT, delim) {}

void bd::HexDataOut::print(std::ostream& os) const
{
    printBuffer(os, [](char* buff, std::size_t len, unsigned char value) noexcept
    {
        to_chars<unsigned char>(buff, buff + len, value, HEX_RADIX);
    });
}

//
// BINARY
//

bd::BinaryDataOut::~BinaryDataOut() = default;

bd::BinaryDataOut::BinaryDataOut(char delim) :
    super(BIN_PREFIX, UCHAR_BIT_COUNT, delim) {}

void bd::BinaryDataOut::print(std::ostream& os) const
{
    printBuffer(os, [](char* buff, std::size_t len, unsigned char value) noexcept
    {
        to_chars<unsigned char>(buff, buff + len, value, BIN_RADIX);
    });
}

//
// BITWISE
//

bd::BitDataOut::~BitDataOut() = default;

bd::BitDataOut::BitDataOut(char delim) :
    super(BIN_PREFIX, UCHAR_BIT_COUNT, delim),
    m_xor(0) {}

int bd::BitDataOut::getDiffPopCount() const
{
    return std::popcount<unsigned char>(m_xor);
}

void bd::BitDataOut::init(std::uintmax_t address, unsigned char dataA, unsigned char dataB) noexcept
{
    super::init(address, dataA, dataB);
    m_xor = dataA ^ dataB;
}

void bd::BitDataOut::print(std::ostream& os) const
{
    printBuffer(os, [x = m_xor](char* buff, std::size_t len, unsigned char value) noexcept
    {
        to_bitwise_string(buff, len, value, x);
    });
}
