/* SPDX-License-Identifier: Apache-2.0 */
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
            virtual ~DataOut();

            virtual int getDiffPopCount() const;

            friend std::ostream& operator<<(std::ostream& os, const DataOut& obj);

        protected:
            DataOut(const unsigned char dataA, const unsigned char dataB, const char delim, char * buffer);

            virtual void print(std::ostream& os) const = 0;

            const unsigned char m_a;
            const unsigned char m_b;
            const unsigned char m_delim;
            char * m_buffer;
        private:
            DataOut() = delete;
            DataOut(const DataOut&) = delete;
            DataOut & operator=(const DataOut&) = delete;
    };

    class HexDataOut final : public DataOut
    {
        public:
            virtual ~HexDataOut();

            // buffer must be at least do_const::UCHAR_HEX_COUNT + 1.
            HexDataOut(const unsigned char dataA, const unsigned char dataB, const char delim, char * buffer);

        protected:
            void print(std::ostream& os) const override;

        private:
            HexDataOut() = delete;
            HexDataOut(const HexDataOut&) = delete;
            HexDataOut & operator=(const HexDataOut&) = delete;

            using super = DataOut;
    };

    class BinaryDataOut final : public DataOut
    {
        public:
            virtual ~BinaryDataOut();

            // buffer must be at least do_const::UCHAR_BIT_COUNT + 1.
            BinaryDataOut(const unsigned char dataA, const unsigned char dataB, const char delim, char * buffer);

        protected:
            void print(std::ostream& os) const override;

        private:
            BinaryDataOut() = delete;
            BinaryDataOut(const BinaryDataOut&) = delete;
            BinaryDataOut & operator=(const BinaryDataOut&) = delete;

            using super = DataOut;
    };

    class BitDataOut final : public DataOut
    {
        public:
            virtual ~BitDataOut();

            // buffer must be at least do_const::UCHAR_BIT_COUNT + 1.
            BitDataOut(const unsigned char dataA, const unsigned char dataB, const char delim, char * buffer);

            int getDiffPopCount() const override;

        protected:
            void print(std::ostream& os) const override;

        private:
            BitDataOut() = delete;
            BitDataOut(const BitDataOut&) = delete;
            BitDataOut & operator=(const BitDataOut&) = delete;

            using super = DataOut;

            const unsigned char m_xor;
    };

    std::ostream& operator<<(std::ostream& os, const DataOut& obj);
}
