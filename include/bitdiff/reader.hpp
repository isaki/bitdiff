/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2025 isaki */

#pragma once

#include <memory>
#include <fstream>
#include <mutex>
#include <filesystem>
#include <cstddef>
#include <condition_variable>
#include <thread>
#include <cstdint>
#include <atomic>

namespace isaki::bitdiff
{
    class Reader final
    {
        public:
            ~Reader();

            // This creates a reader based on a file.
            Reader(const std::filesystem::path& file, const size_t bufferSize);

            // Buffer must be at least as big as the bufferSize used on
            // construction.
            size_t read(unsigned char * buffer);

            bool eof() noexcept;

        private:
            Reader() = delete;
            Reader(const Reader&) = delete;
            Reader & operator=(const Reader&) = delete;

            void run();

            void cleanup() noexcept;

            const size_t m_bsize;

            // Thread control; this is NOT reentrant.
            std::mutex m_mtx;
            std::condition_variable m_bufferFull;
            std::condition_variable m_bufferFree;
            size_t m_read;
            bool m_eof;

            // The thread.
            std::atomic_bool m_stop;
            std::thread * m_thread;

            // The stream
            std::ifstream * m_is;

            // The data
            unsigned char * m_buffer;
    };
}
