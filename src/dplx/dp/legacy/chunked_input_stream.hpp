
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <span>

#include <dplx/cncr/tag_invoke.hpp>

#include <dplx/dp/legacy/memory_buffer.hpp>
#include <dplx/dp/streams/input_buffer.hpp>

namespace dplx::dp::legacy
{

template <typename Impl>
class chunked_input_stream_base : public input_buffer
{
    memory_view mReadArea;

    static constexpr unsigned int small_buffer_size
            = 2 * (minimum_input_buffer_size - 1);

    std::int8_t mBufferStart;
    std::byte mSmallBuffer[small_buffer_size];

protected:
    ~chunked_input_stream_base() noexcept = default;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    explicit chunked_input_stream_base(
            std::span<std::byte const> const initialReadArea,
            std::uint64_t streamSize)
        : input_buffer(
                initialReadArea.data(), initialReadArea.size(), streamSize)
        , mReadArea(initialReadArea)
        , mBufferStart(-1)
    {
    }

    [[nodiscard]] inline auto current_read_area() const noexcept -> memory_view
    {
        memory_view readArea{mReadArea};
        if (mBufferStart < 0)
        {
            readArea.move_consumer_to(static_cast<memory_view::difference_type>(
                    readArea.buffer_size() - size()));
        }
        else if (
                auto const consumedSize = static_cast<int>(
                        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
                        const_cast<input_buffer *>(
                                static_cast<input_buffer const *>(this))
                                ->data()
                        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                        - static_cast<std::byte const *>(mSmallBuffer));
                consumedSize > mBufferStart)
        {
            readArea.move_consumer(consumedSize - mBufferStart);
        }
        return readArea;
    }

private:
    [[nodiscard]] inline auto impl() noexcept -> Impl *
    {
        return static_cast<Impl *>(this);
    }

    inline auto acquire_next_chunk() noexcept -> result<void>
    {
        DPLX_TRY(mReadArea, impl()->acquire_next_chunk_impl(input_size()));
        if (mReadArea.remaining_size() > input_size())
        {
            mReadArea = memory_view{mReadArea.remaining().first(input_size())};
        }
        return success();
    }

    inline void save_remaining_to_small_buffer() noexcept
    {
        auto const remainingSize = size();
        assert(remainingSize <= small_buffer_size);

        std::memcpy(static_cast<std::byte *>(mSmallBuffer), data(),
                    remainingSize);
        mBufferStart += static_cast<std::int8_t>(remainingSize);
        reset(static_cast<std::byte *>(mSmallBuffer), remainingSize,
              input_size());
    }
    inline void append_current_to_small_buffer() noexcept
    {
        auto const smallChunkSize = std::min<std::size_t>(
                mReadArea.remaining_size(), small_buffer_size - size());

        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        std::memcpy(static_cast<std::byte *>(mSmallBuffer) + mBufferStart,
                    mReadArea.remaining_begin(), smallChunkSize);

        reset(static_cast<std::byte *>(mSmallBuffer), size() + smallChunkSize,
              input_size());
    }
    inline void move_small_buffer_content_to_front() noexcept
    {
        auto const consumedSize = static_cast<int>(
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                data() - static_cast<std::byte *>(mSmallBuffer));

        mBufferStart -= static_cast<std::int8_t>(consumedSize);
        std::memmove(static_cast<std::byte *>(mSmallBuffer), data(),
                     static_cast<std::size_t>(mBufferStart));

        reset(static_cast<std::byte *>(mSmallBuffer),
              static_cast<std::size_t>(mBufferStart), input_size());
    }
    inline void retire_small_buffer() noexcept
    {
        auto const consumedSize = static_cast<int>(
                data() - static_cast<std::byte *>(mSmallBuffer));
        mReadArea.move_consumer(consumedSize - mBufferStart);
        mBufferStart = -1;
        reset(mReadArea.remaining_begin(), mReadArea.remaining_size(),
              input_size());
    }

    auto do_require_input(size_type const requiredSize) noexcept
            -> result<void> override
    {
        if (mBufferStart < 0)
        {
            // small buffer is inactive
            if (size() == 0U)
            {
                // we exactly hit the buffer split
                // => no fusing via small buffer necessary
                DPLX_TRY(acquire_next_chunk());
                reset(mReadArea.remaining_begin(), mReadArea.remaining_size(),
                      input_size());

                if (requiredSize > size())
                {
                    return require_input(requiredSize);
                }
                return outcome::success();
            }

            if (requiredSize > small_buffer_size)
            {
                return errc::buffer_size_exceeded;
            }
            mBufferStart = 0;
            save_remaining_to_small_buffer();
            DPLX_TRY(acquire_next_chunk());
            append_current_to_small_buffer();

            if (requiredSize <= size())
            {
                return outcome::success();
            }
            return require_input(requiredSize);
        }

        auto const consumedSize = static_cast<int>(
                data() - static_cast<std::byte *>(mSmallBuffer));
        if (consumedSize >= mBufferStart)
        {
            // remnants from the old buffer have been summarily consumed
            // => switch back to main buffer
            mReadArea.move_consumer(consumedSize - mBufferStart);
            mBufferStart = -1;
            reset(mReadArea.remaining_begin(), mReadArea.remaining_size(),
                  input_size());

            if (requiredSize > mReadArea.remaining_size())
            {
                return require_input(requiredSize);
            }
            return outcome::success();
        }

        if (requiredSize > small_buffer_size)
        {
            return errc::buffer_size_exceeded;
        }

        // there are still remnants of the old buffer, but the read
        // doesn't exceed small_buffer_size.
        move_small_buffer_content_to_front();
        append_current_to_small_buffer();

        if (requiredSize <= size())
        {
            return outcome::success();
        }

        // mReadArea has been completely drained and copied to mSmallBuffer
        mBufferStart = static_cast<std::int8_t>(size());
        mReadArea.reset();
        DPLX_TRY(acquire_next_chunk());
        return require_input(requiredSize);
    }

    auto do_discard_input(size_type discardAmount) noexcept
            -> result<void> override
    {
        if (mBufferStart >= 0)
        {
            retire_small_buffer();
            if (discardAmount < mReadArea.remaining_size())
            {
                discard_buffered(discardAmount);
                return outcome::success();
            }
            discard_buffered(mReadArea.remaining_size());
        }

        do
        {
            DPLX_TRY(acquire_next_chunk());

            auto const discardChunkSize = std::min<size_type>(
                    discardAmount, mReadArea.remaining_size());

            reset(nullptr, 0U, input_size() - discardChunkSize);
            mReadArea.move_consumer(static_cast<memory_buffer::difference_type>(
                    discardChunkSize));
            discardAmount -= discardChunkSize;
        } while (discardAmount != 0U);

        if (mReadArea.remaining_size() == 0U)
        {
            DPLX_TRY(acquire_next_chunk());
        }
        reset(mReadArea.remaining_begin(), mReadArea.remaining_size(),
              input_size());
        return outcome::success();
    }

    auto do_bulk_read(std::byte *dest, std::size_t readAmount) noexcept
            -> result<void> override
    {
        if (mBufferStart >= 0)
        {
            retire_small_buffer();
            return bulk_read(dest, readAmount);
        }

        do
        {
            DPLX_TRY(acquire_next_chunk());

            auto const readChunkSize = std::min<std::size_t>(
                    readAmount, mReadArea.remaining_size());
            std::memcpy(dest, mReadArea.remaining_begin(), readChunkSize);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            dest += readChunkSize;

            reset(nullptr, 0U, input_size() - readChunkSize);
            mReadArea.move_consumer(
                    static_cast<memory_buffer::difference_type>(readChunkSize));
            readAmount -= readChunkSize;
        } while (readAmount != 0);

        if (mReadArea.remaining_size() == 0U)
        {
            DPLX_TRY(acquire_next_chunk());
        }
        reset(mReadArea.remaining_begin(), mReadArea.remaining_size(),
              input_size());
        return outcome::success();
    }
};

} // namespace dplx::dp::legacy
