
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>
#include <cstring>

#include <algorithm>
#include <span>

#include <dplx/dp/byte_buffer.hpp>
#include <dplx/dp/stream.hpp>

namespace dplx::dp
{
template <typename Impl>
class chunked_input_stream_base
{
    const_byte_buffer_view mReadArea;
    std::uint64_t mRemaining;

    static constexpr unsigned int small_buffer_size
            = 2 * (minimum_guaranteed_read_size - 1);
    static constexpr int decommission_threshold = small_buffer_size / 2;

    std::int8_t mBufferStart;
    std::byte mSmallBuffer[small_buffer_size];

protected:
    explicit chunked_input_stream_base(
            std::span<std::byte const> const initialReadArea,
            std::uint64_t streamSize)
        : mReadArea(initialReadArea)
        , mRemaining(streamSize)
        , mBufferStart(-1)
    {
    }

    inline auto current_read_area() const noexcept -> const_byte_buffer_view
    {
        return mReadArea;
    }

private:
    inline auto impl() noexcept -> Impl *
    {
        return static_cast<Impl *>(this);
    }

    inline auto buffered_amount() const noexcept -> unsigned int
    {
        return small_buffer_size - static_cast<unsigned int>(mBufferStart);
    }
    inline auto consume_buffer(std::size_t const amount) noexcept
            -> std::span<std::byte const>
    {
        // precondition:
        // 0 <= mBufferStart < decommission_threshold
        //     => buffered_amount() > minimum_guaranteed_read_size

        auto const remainingBuffered = buffered_amount();
        auto const granted
                = amount <= remainingBuffered ? amount : remainingBuffered;

        std::span<std::byte const> const readProxy(mSmallBuffer + mBufferStart,
                                                   granted);

        mBufferStart += static_cast<std::int8_t>(granted);
        mRemaining -= granted;

        return readProxy;
    }
    inline void decommission_buffer() noexcept
    {
        // precondition:
        // decommission_threshold <= mBufferStart <= small_buffer_size
        auto const consumed = mBufferStart - decommission_threshold;
        mReadArea.move_consumer(consumed);

        mBufferStart = -1;
    }

    inline auto acquire_next_chunk() noexcept -> result<void>
    {
        DPLX_TRY(this->mReadArea, impl()->acquire_next_chunk_impl(mRemaining));
        return success();
    }

    inline auto read(std::size_t const amount) noexcept
            -> result<std::span<std::byte const>>
    {
        if (mBufferStart < 0
            && amount <= static_cast<unsigned int>(mReadArea.remaining_size()))
        {
            mRemaining -= amount;
            return std::span<std::byte const>(
                    mReadArea.consume(static_cast<int>(amount)), amount);
        }

        if (amount > mRemaining)
        {
            return dp::errc::end_of_stream;
        }

        if (mBufferStart < 0)
        {
            auto const remainingChunk
                    = static_cast<unsigned int>(mReadArea.remaining_size());

            if (remainingChunk >= minimum_guaranteed_read_size)
            {
                mRemaining -= amount;
                return std::span<std::byte const>(
                        mReadArea.consume(remainingChunk), remainingChunk);
            }

            auto const bufferStart
                    = static_cast<int>(decommission_threshold - remainingChunk);

            if (remainingChunk > 0)
            {
                std::memcpy(mSmallBuffer + bufferStart,
                            mReadArea.remaining_begin(), remainingChunk);
            }

            DPLX_TRY(this->acquire_next_chunk());

            if (remainingChunk > 0)
            {
                auto const nextPart = std::min(
                        minimum_guaranteed_read_size - 1,
                        static_cast<unsigned int>(mReadArea.remaining_size()));

                std::memcpy(mSmallBuffer + decommission_threshold,
                            mReadArea.remaining_begin(), nextPart);

                mBufferStart = static_cast<int8_t>(bufferStart);
            }

            return read(amount);
        }
        else if (mBufferStart < decommission_threshold)
        {
            return consume_buffer(amount);
        }
        else
        {
            // we ignore the buffer as soon as our read cursor leaves the
            // previous chunk

            decommission_buffer();
            return read(amount);
        }
    }
    auto consume(std::size_t const requestedAmount,
                 std::size_t const actualAmount) noexcept -> result<void>
    {
        auto const unused = static_cast<int>(requestedAmount - actualAmount);

        mRemaining += unused;
        if (mBufferStart < 0)
        {
            mReadArea.move_consumer(-unused);
            return success();
        }
        else
        {
            mBufferStart -= unused;
            return success();
        }
    }

    auto read(std::byte *data, std::size_t const amount) noexcept
            -> result<void>
    {
        // tag_invoke() checks mRemaining > amount

        if (mBufferStart < 0)
        {
            for (std::span<std::byte> remaining(data, amount);
                 remaining.size() > 0;)
            {
                auto const chunk = std::min(
                        remaining.size(),
                        static_cast<std::size_t>(mReadArea.remaining_size()));

                std::memcpy(remaining.data(), mReadArea.consume(chunk), chunk);

                remaining = remaining.subspan(chunk);
                mRemaining -= chunk;

                if (mReadArea.remaining_size() == 0)
                {
                    DPLX_TRY(this->acquire_next_chunk());
                }
            }

            return success();
        }
        else if (mBufferStart < decommission_threshold)
        {
            auto buffered = consume_buffer(amount);

            std::memcpy(data, buffered.data(), buffered.size());

            if (buffered.size() == amount)
            {
                return success();
            }
            else
            {
                // need more data than buffered
                decommission_buffer();
                return read(data + buffered.size(), amount - buffered.size());
            }
        }
        else
        {
            decommission_buffer();
            return read(data, amount);
        }
    }

public:
    friend inline auto tag_invoke(tag_t<dp::available_input_size>,
                                  chunked_input_stream_base &self) noexcept
            -> result<std::uint64_t>
    {
        return self.mRemaining;
    }
    friend inline auto tag_invoke(tag_t<dp::read>,
                                  chunked_input_stream_base &self,
                                  std::size_t const amount) noexcept
            -> result<std::span<std::byte const>>
    {
        return self.read(amount);
    }
    friend inline auto tag_invoke(tag_t<dp::consume>,
                                  chunked_input_stream_base &self,
                                  std::span<std::byte const> proxy,
                                  std::size_t const actualAmount) noexcept
            -> result<void>
    {
        return self.consume(proxy.size(), actualAmount);
    }
    friend inline auto tag_invoke(tag_t<dp::read>,
                                  chunked_input_stream_base &self,
                                  std::byte *buffer,
                                  std::size_t const amount) noexcept
            -> result<void>
    {
        if (amount > self.mRemaining)
        {
            return dp::errc::end_of_stream;
        }

        return self.read(buffer, amount);
    }
};
} // namespace dplx::dp
