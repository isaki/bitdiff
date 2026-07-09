/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright 2025-2026 isaki */

#pragma once

#include <iostream>
#include <cstddef>
#include <cstdint>
#include <climits>

#include <concepts>
#include <type_traits>

namespace isaki::bitdiff
{
    namespace internal
    {
        template <typename F>
        concept BuildFunction = requires(F f, char* ptr, std::size_t size, unsigned char value)
        {
            { f(ptr, size, value) } -> std::same_as<void>;
        };
    }

    inline constexpr std::size_t UCHAR_HEX_COUNT = sizeof(unsigned char) * (CHAR_BIT >> 2);
    inline constexpr std::size_t UCHAR_BIT_COUNT = sizeof(unsigned char) * CHAR_BIT;
    inline constexpr std::size_t UINTMAX_HEX_COUNT = sizeof(std::uintmax_t) * (CHAR_BIT >> 2);

    enum class DataOutType
    {
        Hex,
        Binary,
        Bits
    };

    class DataOut
    {
    public:
        DataOut() = delete;
        DataOut(const DataOut&) = delete;
        DataOut & operator=(const DataOut&) = delete;
        DataOut(DataOut&& o) = delete;
        DataOut & operator=(DataOut&& o) = delete;

        virtual ~DataOut();

        [[nodiscard]] virtual int getDiffPopCount() const;

        virtual void init(unsigned char dataA, unsigned char dataB) noexcept;

        virtual void print(std::ostream& os) const = 0;

    protected:
        DataOut(std::string_view prefix, std::size_t tokenSize, char delim);

        template<typename F>
        requires internal::BuildFunction<F>
        void printBuffer(std::ostream& os, F&& f) const
        {
            // Fill the buffer correctly.
            f(m_posA, m_tokenSize, m_a);
            f(m_posB, m_tokenSize, m_b);

            os << m_buffer;
        }

    private:
        std::size_t m_tokenSize;
        mutable char* m_buffer;
        mutable char* m_posA;
        mutable char* m_posB;
        unsigned char m_a;
        unsigned char m_b;
    };

    class HexDataOut final : public DataOut
    {
    public:
        HexDataOut() = delete;
        HexDataOut(const HexDataOut&) = delete;
        HexDataOut & operator=(const HexDataOut&) = delete;
        HexDataOut(HexDataOut&& o) = delete;
        HexDataOut & operator=(HexDataOut&& o) = delete;

        ~HexDataOut() override;

        explicit HexDataOut(char delim);

        void print(std::ostream& os) const override;

    private:
        using super = DataOut;
    };

    class BinaryDataOut final : public DataOut
    {
    public:
        BinaryDataOut() = delete;
        BinaryDataOut(const BinaryDataOut&) = delete;
        BinaryDataOut & operator=(const BinaryDataOut&) = delete;
        BinaryDataOut(BinaryDataOut&& o) = delete;
        BinaryDataOut & operator=(BinaryDataOut&& o) = delete;

        ~BinaryDataOut() override;

        explicit BinaryDataOut(char delim);

        void print(std::ostream& os) const  override;

    private:
        using super = DataOut;
    };

    class BitDataOut final : public DataOut
    {
    public:
        BitDataOut() = delete;
        BitDataOut(const BitDataOut&) = delete;
        BitDataOut & operator=(const BitDataOut&) = delete;
        BitDataOut(BitDataOut&& o) = delete;
        BitDataOut & operator=(BitDataOut&& o) = delete;

        ~BitDataOut() override;

        BitDataOut(char delim);

        [[nodiscard]] int getDiffPopCount() const override;

        void init(unsigned char dataA, unsigned char dataB) noexcept override;

        void print(std::ostream& os) const  override;

    private:
        unsigned char m_xor;

        using super = DataOut;
    };

    inline std::ostream& operator<<(std::ostream& os, const DataOut& obj)
    {
        obj.print(os);
        return os;
    }
}
