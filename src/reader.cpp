/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright 2025-2026 isaki */

#include <mutex>
#include <thread>
#include <stop_token>

#include <condition_variable>

#include <cstddef>
#include <cstring>

#include <filesystem>
#include <fstream>
#include <iostream>

#include <exception>
#include <stdexcept>

#include "bitdiff/reader.hpp"

namespace bd = isaki::bitdiff;
namespace fs = std::filesystem;

namespace
{
    std::streamsize fillBuffer(std::ifstream * in, unsigned char* buffer, const std::streamsize len)
    {
        char* sbuff = reinterpret_cast<char*>(buffer);

        std::streamsize read = 0;
        while (read < len && !in->eof())
        {
            in->read(sbuff + read, len - read);
            read += in->gcount();
        }

        return read;
    }
}

bd::Reader::~Reader()
{
    // Create the lock, but unlocked
    // This will wake the producer (blocking on std::condition_variable_any)
    // The producer will mark m_eos, then notify all waiting threads.
    // The extra notify isn't needed really, since this should be called from
    // withing the consumer thread. In the event the main thread becomes orchestration,
    // this still works, because the notify will wake the consumer(s) and m_eos will be set,
    // they will be done, and will join.
    m_thread.request_stop();

    // Join the threads.
    m_thread.join();

    // Teardown everything else.
    cleanup();
}

bd::Reader::Reader(const fs::path& file, const std::size_t bufferSize) :
    m_bsize(bufferSize),
    m_error(nullptr),
    m_read(0),
    m_eos(false),
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
        m_thread = std::jthread([this](std::stop_token stop) { this->run(stop); });
    }
    catch (const std::exception& e)
    {
        std::cerr << "Reader initialization failure: " << e.what() << std::endl;

        // Cleanup will clear valid flag. We don't need lock. If thread threw,
        // we have no thread anyway.
        cleanup();
        throw;
    }
}

std::size_t bd::Reader::read(unsigned char* buffer)
{
    // This is effectively a consumer.
    std::unique_lock<std::mutex> lock(m_mtx);

    m_bufferFull.wait(lock, [this]
    {
        return this->m_read > 0 || this->m_eos;
    });

    if (m_error) [[unlikely]]
    {
        std::rethrow_exception(m_error);
    }

    std::streamsize ret = 0;
    if (m_read > 0)
    {
        std::memcpy(buffer, m_buffer, static_cast<std::size_t>(m_read));
        ret = m_read;
        m_read = 0;
    }

    m_bufferFree.notify_one();

    return static_cast<std::size_t>(ret);
}

void bd::Reader::run(std::stop_token stop)
{
    try
    {
        for (;;)
        {
            // This is the producer and the thread.
            std::unique_lock<std::mutex> lock(m_mtx);
            m_bufferFree.wait(lock, stop, [this] { return this->m_read == 0; });

            if (stop.stop_requested())
            {
                m_eos = true;
                m_bufferFull.notify_all();
                break;
            }

            m_read = fillBuffer(m_is, m_buffer, static_cast<std::streamsize>(m_bsize));

            if (m_read == 0)
            {
                m_eos = true;
                m_bufferFull.notify_all();
                break;
            }

            // else
            m_bufferFull.notify_one();
        }
    }
    catch (...)
    {
        std::scoped_lock<std::mutex> lock(m_mtx);
        m_error = std::current_exception();
        m_eos = true;
        m_bufferFull.notify_all();
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
}
