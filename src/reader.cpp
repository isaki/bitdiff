/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2025 isaki */

#include <mutex>
#include <thread>
#include <memory>
#include <cstddef>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <condition_variable>

#include "bitdiff/reader.hpp"

namespace bd = isaki::bitdiff;
namespace fs = std::filesystem;

namespace
{
    struct _read_summary_s final
    {
        size_t bytesRead;
        bool eof;
    };

    struct _read_summary_s _fillBuffer(std::ifstream * in, unsigned char * buffer, const size_t len)
    {
        char * sbuff = reinterpret_cast<char *>(buffer);

        size_t read = 0;
        while (read < len && !in->eof())
        {
            in->read(sbuff, len - read);
            read += in->gcount();
        }

        return { .bytesRead = read, .eof = in->eof() };
    }
}

bd::Reader::~Reader()
{
    // Shared resources need to be protected.
    std::unique_lock<std::mutex> lock(m_mtx);
    cleanup();
}

bd::Reader::Reader(const fs::path& file, const size_t bufferSize) :
    m_bsize(bufferSize),
    m_read(0),
    m_eof(false),
    m_is(nullptr),
    m_buffer(nullptr)
{
    try
    {
        // First can we even create the stream?
        m_is = new std::ifstream();
        m_is->open(file, std::ios_base::binary | std::ios_base::in);
        if (!m_is->is_open())
        {
            std::string err;
            err.append("Unable to open ");
            err.append(file.string());
            throw std::runtime_error(err);
        }

        m_is->exceptions(std::ifstream::badbit);

        // We don't need to zero memory here.
        m_buffer = new unsigned char[bufferSize];

        m_jthread = std::jthread(&bd::Reader::run, this);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Reader initialization failure: " << e.what() << std::endl;

        // Cleanup will clear valid flag. We don't need lock. If jthread threw,
        // we have no thread anyway.
        cleanup();
        throw e;
    }
}

size_t bd::Reader::read(unsigned char * buffer)
{
    // This is effectively a consumer.
    std::unique_lock<std::mutex> lock(m_mtx);
    m_bufferFull.wait(lock, [this] { return this->m_read > 0 || this->m_eof; });

    size_t ret = 0;
    if (m_read > 0)
    {
        memcpy(buffer, m_buffer, m_read);
        ret = m_read;
        m_read = 0;
    }

    m_bufferFree.notify_one();

    return ret;
}

bool bd::Reader::eof() noexcept
{
    std::unique_lock<std::mutex> lock(m_mtx);
    return m_eof;
}

void bd::Reader::run()
{
    for (;;)
    {
        // This is the producer and the thread.
        std::unique_lock<std::mutex> lock(m_mtx);
        m_bufferFree.wait(lock, [this] { return this->m_read == 0; });

        struct _read_summary_s summary = _fillBuffer(m_is, m_buffer, m_bsize);
        m_read = summary.bytesRead;

        m_bufferFull.notify_one();

        if (summary.eof)
        {
            // Mark EOF and terminate the thread
            m_eof = summary.eof;
            break;
        }
    }

    // End of thread reached.
}

// This is NOT thread safe.
void bd::Reader::cleanup() noexcept
{
    // This may not throw exceptions.
    if (m_buffer != nullptr)
    {
        delete[] m_buffer;
        m_buffer = nullptr;
    }

    if (m_is != nullptr)
    {
        try
        {
            if (m_is->is_open())
            {
                m_is->close();
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "Failed to close stream: " << e.what() << std::endl;
        }

        delete m_is;
        m_is = nullptr;
    }

    m_eof = true;
}
