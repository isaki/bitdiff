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
#include <atomic>

#include "bitdiff/reader.hpp"

namespace bd = isaki::bitdiff;
namespace fs = std::filesystem;

namespace
{
    std::streamsize _fillBuffer(std::ifstream * in, unsigned char * buffer, const std::streamsize len)
    {
        char * sbuff = reinterpret_cast<char *>(buffer);

        std::streamsize read = 0;
        while (read < len && !in->eof())
        {
            in->read(sbuff, len - read);
            read += in->gcount();
        }

        return read;
    }
}

bd::Reader::~Reader()
{
    // Create the lock, but unlocked
    std::unique_lock<std::mutex> lock(m_mtx, std::defer_lock);

    // Set the atomic boolean before taking the lock.
    m_stop = true;

    // Lock for safe data access.
    lock.lock();
    if (m_thread != nullptr)
    {
        m_bufferFree.notify_all();
        m_bufferFull.notify_all();

        // We need this released so threads can cleanup.
        lock.unlock();

        m_thread->join();

        // We can now restore the lock state.
        lock.lock();
        delete m_thread;
        m_thread = nullptr;
    }

    cleanup();

    // Lock releases when going out of scope.
}

bd::Reader::Reader(const fs::path& file, const size_t bufferSize) :
    m_bsize(bufferSize),
    m_read(0),
    m_eof(false),
    m_stop(false),
    m_thread(nullptr),
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

        // This must be the last call before the end of the try block.
        m_thread = new std::thread(&bd::Reader::run, this);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Reader initialization failure: " << e.what() << std::endl;

        // Cleanup will clear valid flag. We don't need lock. If thread threw,
        // we have no thread anyway.
        cleanup();
        throw e;
    }
}

size_t bd::Reader::read(unsigned char * buffer)
{
    // This is effectively a consumer.
    std::unique_lock<std::mutex> lock(m_mtx);
    m_bufferFull.wait(lock, [this] { return this->m_read > 0 || this->m_eof || this->m_stop.load(); });

    if (m_stop.load())
    {
        throw std::runtime_error("Unexpected reader thread termination");
    }

    std::streamsize ret = 0;
    if (m_read > 0)
    {
        memcpy(buffer, m_buffer, static_cast<size_t>(m_read));
        ret = m_read;
        m_read = 0;
    }

    m_bufferFree.notify_all();

    return static_cast<size_t>(ret);
}

bool bd::Reader::eof() noexcept
{
    std::unique_lock<std::mutex> lock(m_mtx);
    return m_eof;
}

void bd::Reader::run()
{
    while (! m_stop.load())
    {
        // This is the producer and the thread.
        std::unique_lock<std::mutex> lock(m_mtx);
        m_bufferFree.wait(lock, [this] { return this->m_read == 0 || this->m_stop.load(); });

        if (m_stop.load())
        {
            break;
        }

        m_read = _fillBuffer(m_is, m_buffer, static_cast<std::streamsize>(m_bsize));

        m_bufferFull.notify_all();

        if (m_is->eof())
        {
            // Mark EOF and terminate the thread
            m_eof = true;
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
