
// Copyright Henrik Steffen Gaßmann 2020.
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>
#include <cstring>
#include <span>

#include <dplx/cncr/tag_invoke.hpp>
#include <dplx/predef/compiler.h>

#include <dplx/dp/streams/output_buffer.hpp>

namespace dplx::dp::legacy
{

template <typename Impl>
class chunked_output_stream_base : public output_buffer
{
    friend Impl;

    std::span<std::byte> mCurrentChunk;
    size_type mRemaining;

    static constexpr unsigned int small_buffer_size
            = 2 * (minimum_output_buffer_size - 1);

    std::int8_t mDecomissionThreshold;
    std::byte mSmallBuffer[small_buffer_size];

protected:
    ~chunked_output_stream_base() noexcept = default;

private:
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    chunked_output_stream_base(std::span<std::byte> initialWriteArea,
                               size_type remaining) noexcept
        : output_buffer(initialWriteArea.data(), initialWriteArea.size())
        , mCurrentChunk(initialWriteArea)
        , mRemaining(remaining)
        , mDecomissionThreshold(-1)
        , mSmallBuffer{}
    {
    }

    auto impl() noexcept -> Impl *
    {
        return static_cast<Impl *>(this);
    }

#if defined(DPLX_COMP_GNUC_AVAILABLE) && !defined(DPLX_COMP_CLANG_AVAILABLE)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuseless-cast" // std::size_t casts :(
#endif
    auto acquire_next_chunk() noexcept -> result<void>
    {
        using byte_span = std::span<std::byte>;
        DPLX_TRY(byte_span const nextChunk, impl()->acquire_next_chunk_impl());

        mCurrentChunk = nextChunk.size() > mRemaining
                                ? nextChunk.first(
                                          static_cast<std::size_t>(mRemaining))
                                : nextChunk;

        mRemaining -= mCurrentChunk.size();

        return outcome::success();
    }
#if defined(DPLX_COMP_GNUC_AVAILABLE) && !defined(DPLX_COMP_CLANG_AVAILABLE)
#pragma GCC diagnostic pop
#endif

    auto do_grow(size_type requestedSize) noexcept -> result<void> override
    {
        if (mDecomissionThreshold < 0)
        {
            if (size() == 0U)
            {
                DPLX_TRY(acquire_next_chunk());
                reset(mCurrentChunk.data(), mCurrentChunk.size());
                return outcome::success();
            }
            mDecomissionThreshold = static_cast<std::int8_t>(size());
            reset(static_cast<std::byte *>(mSmallBuffer), small_buffer_size);
            if (requestedSize > small_buffer_size)
            {
                return errc::buffer_size_exceeded;
            }
            return outcome::success();
        }

        auto const chunkPart = mCurrentChunk.last(
                static_cast<std::size_t>(mDecomissionThreshold));
        auto const consumedSize
                = data() - static_cast<std::byte *>(mSmallBuffer);
        if (consumedSize < mDecomissionThreshold)
        {
            std::memcpy(chunkPart.data(),
                        static_cast<std::byte *>(mSmallBuffer),
                        static_cast<std::size_t>(consumedSize));

            reset(static_cast<std::byte *>(mSmallBuffer), small_buffer_size);
            mDecomissionThreshold -= static_cast<std::int8_t>(consumedSize);
            if (requestedSize > small_buffer_size)
            {
                return errc::buffer_size_exceeded;
            }
            return outcome::success();
        }

        if (!chunkPart.empty()) [[likely]]
        {
            std::memcpy(chunkPart.data(),
                        static_cast<std::byte *>(mSmallBuffer),
                        chunkPart.size());
        }

        auto const overlap = consumedSize - mDecomissionThreshold;
        if (auto acquireRx = acquire_next_chunk(); acquireRx.has_error())
                [[unlikely]]
        {
            move_remaining_small_buffer_to_front(
                    static_cast<std::size_t>(overlap));
            return std::move(acquireRx).assume_error();
        }

        if (!try_move_small_buffer_to_next_chunk(
                    static_cast<std::size_t>(overlap))
            && mRemaining == 0)
        {
            return errc::end_of_stream;
        }
        return ensure_size(requestedSize);
    }
    auto do_bulk_write(std::byte const *src, std::size_t writeAmount) noexcept
            -> result<void> override
    {
        if (mDecomissionThreshold >= 0)
        {
            auto const chunkPart = mCurrentChunk.last(
                    static_cast<std::size_t>(mDecomissionThreshold));
            std::memcpy(chunkPart.data(),
                        static_cast<std::byte *>(mSmallBuffer),
                        chunkPart.size());

            // we can use small_buffer_size here as bulk_write is guaranteed to
            // fill the buffer completely
            // => small_buffer_size == data() - mSmallBuffer
            auto const overlap
                    = small_buffer_size
                      - static_cast<std::size_t>(mDecomissionThreshold);

            if (auto acquireRx = acquire_next_chunk(); acquireRx.has_error())
                    [[unlikely]]
            {
                move_remaining_small_buffer_to_front(overlap);
                return std::move(acquireRx).assume_error();
            }

            if (!try_move_small_buffer_to_next_chunk(overlap)) [[unlikely]]
            {
                DPLX_TRY(acquire_next_chunk());
            }
            reset();
        }
        else
        {
            DPLX_TRY(acquire_next_chunk());
        }

        do
        {
            auto const chunkSize
                    = std::min<std::size_t>(writeAmount, mCurrentChunk.size());
            std::memcpy(mCurrentChunk.data(), src, chunkSize);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            src += chunkSize;
            writeAmount -= chunkSize;

            if (chunkSize == mCurrentChunk.size())
            {
                DPLX_TRY(acquire_next_chunk());
            }
            else
            {
                mCurrentChunk = mCurrentChunk.subspan(chunkSize);
            }
        }
        while (writeAmount != 0);
        reset(mCurrentChunk.data(), mCurrentChunk.size());
        return outcome::success();
    }

    auto do_sync_output() noexcept -> result<void> override
    {
        if (mDecomissionThreshold < 0)
        {
            // small buffer is not in use => nothing to do
            return outcome::success();
        }

        auto const chunkPart = mCurrentChunk.last(
                static_cast<std::size_t>(mDecomissionThreshold));
        auto const consumedSize
                = data() - static_cast<std::byte *>(mSmallBuffer);
        if (consumedSize <= mDecomissionThreshold)
        {
            // written data still does not exceed current chunk
            std::memcpy(chunkPart.data(),
                        static_cast<std::byte *>(mSmallBuffer),
                        static_cast<std::size_t>(consumedSize));
            if (consumedSize == mDecomissionThreshold)
            {
                // avoid acquiring a new chunk as sync_output is usually called
                // as a cleanup operation and a new chunk would go to waste
                // and/or an illegal operation in case of a pre-sized stream
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                reset(mCurrentChunk.data() + mCurrentChunk.size(), 0U);
                mDecomissionThreshold = -1;
            }
            else
            {
                reset(static_cast<std::byte *>(mSmallBuffer),
                      small_buffer_size);
                mDecomissionThreshold -= static_cast<std::int8_t>(consumedSize);
            }
            return outcome::success();
        }

        std::memcpy(chunkPart.data(), static_cast<std::byte *>(mSmallBuffer),
                    chunkPart.size());
        auto const overlap = consumedSize - mDecomissionThreshold;
        if (auto acquireRx = acquire_next_chunk(); acquireRx.has_error())
                [[unlikely]]
        {
            move_remaining_small_buffer_to_front(
                    static_cast<std::size_t>(overlap));
            return std::move(acquireRx).assume_error();
        }

        if (!try_move_small_buffer_to_next_chunk(
                    static_cast<std::size_t>(overlap)))
        {
            return errc::end_of_stream;
        }
        return outcome::success();
    }

    auto
    try_move_small_buffer_to_next_chunk(std::size_t const remaining) noexcept
            -> bool
    {
        auto const copyAmount = std::min(remaining, mCurrentChunk.size());
        if (mCurrentChunk.data() != nullptr) [[likely]]
        {
            std::memcpy(
                    mCurrentChunk.data(),
                    static_cast<std::byte *>(mSmallBuffer)
                            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                            + mDecomissionThreshold,
                    remaining);
            mCurrentChunk = mCurrentChunk.subspan(copyAmount);
        }
        if (remaining != copyAmount) [[unlikely]]
        {
            mDecomissionThreshold += static_cast<std::int8_t>(copyAmount);
            move_remaining_small_buffer_to_front(remaining - copyAmount);
            return false;
        }
        reset(mCurrentChunk.data(), mCurrentChunk.size());
        mDecomissionThreshold = -1;
        return true;
    }

    void
    move_remaining_small_buffer_to_front(std::size_t const remaining) noexcept
    {
        std::memmove(
                static_cast<std::byte *>(mSmallBuffer),
                static_cast<std::byte *>(mSmallBuffer)
                        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                        + mDecomissionThreshold,
                remaining);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        reset(static_cast<std::byte *>(mSmallBuffer) + remaining,
              small_buffer_size - remaining);
        mDecomissionThreshold = 0;
    }
};

} // namespace dplx::dp::legacy
