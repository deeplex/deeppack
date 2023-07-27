
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>
#include <cstring>
#include <initializer_list>
#include <vector>

#include <dplx/dp/items/emit_context.hpp>
#include <dplx/dp/streams/output_buffer.hpp>

#include "test_utils.hpp"

namespace dp_tests
{

// the class is final and none of its base classes have public destructors
// NOLINTNEXTLINE(cppcoreguidelines-virtual-class-destructor)
class simple_test_output_stream final : public dp::output_buffer
{
    std::vector<std::byte> mBuffer;
    bool mInitiallyEmpty;

public:
    ~simple_test_output_stream() = default;

    simple_test_output_stream(std::size_t const bufferSize,
                              bool const initiallyEmpty = false)
        : output_buffer()
        , mBuffer(bufferSize, std::byte{})
        , mInitiallyEmpty(initiallyEmpty)
    {
        if (!initiallyEmpty)
        {
            reset(mBuffer.data(), bufferSize);
        }
    }

    [[nodiscard]] auto written() const noexcept -> std::span<std::byte const>
    {
        std::span<std::byte const> bytes{};
        if (!mInitiallyEmpty)
        {
            bytes = mBuffer;
            bytes = bytes.first(mBuffer.size() - size());
        }
        return bytes;
    }

private:
    auto do_grow(size_type const requested) noexcept
            -> dp::result<void> override
    {
        if (!std::exchange(mInitiallyEmpty, false)
            || requested > mBuffer.size())
        {
            return dp::errc::end_of_stream;
        }
        reset(mBuffer.data(), mBuffer.size());
        return dp::oc::success();
    }

    auto do_bulk_write(std::byte const *const src,
                       std::size_t const srcSize) noexcept
            -> dp::result<void> override
    {
        if (!std::exchange(mInitiallyEmpty, false) || srcSize > mBuffer.size())
        {
            return dp::errc::end_of_stream;
        }
        std::memcpy(mBuffer.data(), src, srcSize);
        if (srcSize != mBuffer.size())
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            reset(mBuffer.data() + srcSize, mBuffer.size() - srcSize);
        }
        return dp::oc::success();
    }
};

class simple_test_emit_context final : private dp::emit_context
{
public:
    simple_test_output_stream stream;

    explicit simple_test_emit_context(std::size_t const bufferSize,
                                      bool const initiallyEmpty = false)
        : emit_context{stream}
        , stream(bufferSize, initiallyEmpty)
    {
    }

    auto as_emit_context() noexcept -> dp::emit_context &
    {
        return *this;
    }
};

// the class is final and none of its base classes have public destructors
// NOLINTNEXTLINE(cppcoreguidelines-virtual-class-destructor)
class test_output_stream final : public dp::output_buffer
{
    using buffers_type = std::vector<std::vector<std::byte>>;
    buffers_type mGatherBuffers;
    typename buffers_type::iterator mCurrentBuffer;
    bool mInitiallyEmpty;

public:
    ~test_output_stream() noexcept = default;

    explicit test_output_stream(
            std::initializer_list<std::size_t> const gatherBufferSizes,
            bool const initiallyEmpty = false)
        : output_buffer()
        , mGatherBuffers(allocate_buffers(gatherBufferSizes))
        , mCurrentBuffer(mGatherBuffers.begin())
        , mInitiallyEmpty(initiallyEmpty)
    {
        if (!initiallyEmpty && mCurrentBuffer != mGatherBuffers.end())
        {
            auto &startBuffer = *mCurrentBuffer;
            reset(startBuffer.data(), startBuffer.size());
        }
    }

    [[nodiscard]] auto written() const -> std::vector<std::byte>
    {
        std::vector<std::byte> bytes;
        if (mInitiallyEmpty)
        {
            // do_grow() hasn't been called
        }
        else if (!mGatherBuffers.empty())
        {
            std::size_t accumulatedSize = 0U;
            auto gatherBuffersIt = mGatherBuffers.begin();
            for (; gatherBuffersIt != mCurrentBuffer; ++gatherBuffersIt)
            {
                accumulatedSize += gatherBuffersIt->size();
            }
            if (gatherBuffersIt != mGatherBuffers.end())
            {
                accumulatedSize += gatherBuffersIt->size() - size();
            }
            bytes.resize(accumulatedSize);

            gatherBuffersIt = mGatherBuffers.begin();
            auto *writePos = bytes.data();
            for (; gatherBuffersIt != mCurrentBuffer; ++gatherBuffersIt)
            {
                std::memcpy(writePos, gatherBuffersIt->data(),
                            gatherBuffersIt->size());
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                writePos += gatherBuffersIt->size();
            }
            if (gatherBuffersIt != mGatherBuffers.end())
            {
                auto const lastBufferSize = gatherBuffersIt->size() - size();
                std::memcpy(writePos, gatherBuffersIt->data(), lastBufferSize);
            }
        }
        return bytes;
    }

private:
    static auto allocate_buffers(std::initializer_list<std::size_t> bufferSizes)
            -> std::vector<std::vector<std::byte>>
    {
        std::vector<std::vector<std::byte>> gatherBuffers;
        gatherBuffers.reserve(bufferSizes.size());
        for (auto const gatherBufferSize : bufferSizes)
        {
            gatherBuffers.emplace_back(gatherBufferSize);
        }
        return gatherBuffers;
    }

    auto do_grow(size_type const requestedSize) noexcept
            -> dp::result<void> override
    {
        if (!std::exchange(mInitiallyEmpty, false))
        {
            finalize_current_buffer();
        }
        for (; mCurrentBuffer != mGatherBuffers.end(); ++mCurrentBuffer)
        {
            auto &currentBuffer = *mCurrentBuffer;
            if (currentBuffer.size() < requestedSize)
            {
                currentBuffer.resize(0U);
            }
            else
            {
                reset(currentBuffer.data(), currentBuffer.size());
                return dp::oc::success();
            }
        }
        reset();
        return dp::errc::end_of_stream;
    }
    auto do_bulk_write(std::byte const *src, std::size_t size) noexcept
            -> dp::result<void> override
    {
        if (!std::exchange(mInitiallyEmpty, false))
        {
            finalize_current_buffer();
        }
        for (; size > 0U && mCurrentBuffer != mGatherBuffers.end();
             ++mCurrentBuffer)
        {
            auto &currentBuffer = *mCurrentBuffer;
            auto chunkSize = std::min(size, currentBuffer.size());
            if (chunkSize == 0U)
            {
                // skip empty buffers
                continue;
            }

            std::memcpy(currentBuffer.data(), src, chunkSize);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            src += chunkSize;
            size -= chunkSize;

            if (auto const remainingSize = currentBuffer.size() - chunkSize;
                remainingSize > 0U)
            {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                reset(currentBuffer.data() + chunkSize, remainingSize);
            }
        }
        if (size > 0U)
        {
            return dp::errc::end_of_stream;
        }
        return dp::oc::success();
    }

    void finalize_current_buffer() noexcept
    {
        auto const remainingSize = size();
        auto const bufferSize = mCurrentBuffer->size();
        mCurrentBuffer->resize(bufferSize - remainingSize);
        ++mCurrentBuffer;
    }
};

class test_emit_context final
{
public:
    // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
    test_output_stream stream;

private:
    dp::emit_context ctx;

public:
    explicit test_emit_context(
            std::initializer_list<std::size_t> const gatherBufferSizes,
            bool const initiallyEmpty = false)
        : stream(gatherBufferSizes, initiallyEmpty)
        , ctx{stream}
    {
    }

    auto as_emit_context() noexcept -> dp::emit_context &
    {
        return ctx;
    }
};

} // namespace dp_tests
