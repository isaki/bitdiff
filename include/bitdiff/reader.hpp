/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright 2025-2026 isaki */

#pragma once

#include <memory>
#include <fstream>
#include <mutex>
#include <filesystem>
#include <cstddef>
#include <condition_variable>
#include <thread>
#include <exception>

namespace isaki::bitdiff
{
    class Reader final
    {
    public:
        Reader() = delete;
        Reader(const Reader&) = delete;
        Reader& operator=(const Reader&) = delete;
        Reader(Reader&&) = delete;
        Reader& operator=(Reader&&) = delete;

        ~Reader();

        // This creates a reader based on a file.
        Reader(const std::filesystem::path& file, std::size_t bufferSize);

        // Buffer must be at least as big as the bufferSize used on
        // construction.
        std::size_t read(unsigned char * buffer);

    private:

        void run(std::stop_token stop);

        void cleanup() noexcept;

        const std::size_t m_bsize;

        // Additional error tracking
        std::exception_ptr m_error;

        // Thread control; this is NOT reentrant.
        std::mutex m_mtx;
        std::condition_variable m_bufferFull;
        std::condition_variable m_bufferFree;
        std::streamsize m_read;
        bool m_eos;

        // The stream
        std::ifstream* m_is;

        // The data
        unsigned char* m_buffer;

        // Thread must outlive resources used by run()
        std::jthread m_thread;
    };
}
