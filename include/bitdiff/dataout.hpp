/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright 2025 isaki */

#pragma once

#include <iostream>
#include <cstddef>
#include <cstdint>
#include <climits>

namespace isaki::bitdiff
{
    inline constexpr size_t UCHAR_HEX_COUNT = sizeof(unsigned char) * (CHAR_BIT >> 2);
    inline constexpr size_t UCHAR_BIT_COUNT = sizeof(unsigned char) * CHAR_BIT;
    inline constexpr size_t UINTMAX_HEX_COUNT = sizeof(uintmax_t) * (CHAR_BIT >> 2);

    enum class DataOutType
    {
        Hex,
        Binary,
        Bits
    };

    class DataOut {
        public:
            DataOut() = delete;
            DataOut(const DataOut&) = delete;
            DataOut & operator=(const DataOut&) = delete;

            virtual ~DataOut();

            [[nodiscard]] virtual int getDiffPopCount() const;

            virtual void init(unsigned char dataA, unsigned char dataB) noexcept;

            friend std::ostream& operator<<(std::ostream& os, const DataOut& obj);

        protected:
            DataOut(char delim, size_t bufferSize);

            virtual void print(std::ostream& os) const = 0;

            const unsigned char m_delim;
            unsigned char m_a;
            unsigned char m_b;
            char * m_buffer;
    };

    class HexDataOut final : public DataOut
    {
        public:
            HexDataOut() = delete;
            HexDataOut(const HexDataOut&) = delete;
            HexDataOut & operator=(const HexDataOut&) = delete;

            ~HexDataOut() override;

            explicit HexDataOut(char delim);

        protected:
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

            ~BinaryDataOut() override;

            explicit BinaryDataOut(char delim);

        protected:
            void print(std::ostream& os) const override;

        private:
            using super = DataOut;
    };

    class BitDataOut final : public DataOut
    {
        public:
            BitDataOut() = delete;
            BitDataOut(const BitDataOut&) = delete;
            BitDataOut & operator=(const BitDataOut&) = delete;

            ~BitDataOut() override;

            explicit BitDataOut(char delim);

            [[nodiscard]] int getDiffPopCount() const override;

            void init(unsigned char dataA, unsigned char dataB) noexcept override;

        protected:
            void print(std::ostream& os) const override;

        private:
            unsigned char m_xor;

            using super = DataOut;
    };

    std::ostream& operator<<(std::ostream& os, const DataOut& obj);
}
