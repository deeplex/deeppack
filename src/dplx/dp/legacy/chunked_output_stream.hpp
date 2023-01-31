
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
    std::span<std::byte> mCurrentChunk;
    size_type mRemaining;

    static constexpr unsigned int small_buffer_size
            = 2 * (minimum_output_buffer_size - 1);

    std::int8_t mDecomissionThreshold;
    std::byte mSmallBuffer[small_buffer_size];

protected:
    ~chunked_output_stream_base() noexcept = default;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    chunked_output_stream_base(std::span<std::byte> initialWriteArea,
                               size_type remaining) noexcept
        : output_buffer(initialWriteArea.data(), initialWriteArea.size())
        , mCurrentChunk(initialWriteArea)
        , mRemaining(remaining)
        , mDecomissionThreshold(-1)
    {
    }

private:
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

        mCurrentChunk
                = nextChunk.size() > mRemaining
                        ? nextChunk.first(static_cast<std::size_t>(mRemaining))
                        : nextChunk;

        mRemaining -= mCurrentChunk.size();

        return oc::success();
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
                return oc::success();
            }
            if (requestedSize > small_buffer_size)
            {
                return errc::buffer_size_exceeded;
            }
            mDecomissionThreshold = static_cast<std::int8_t>(size());
            reset(static_cast<std::byte *>(mSmallBuffer), small_buffer_size);
            return oc::success();
        }

        auto const chunkPart = mCurrentChunk.last(
                static_cast<std::size_t>(mDecomissionThreshold));
        auto const consumedSize = static_cast<std::size_t>(
                data() - static_cast<std::byte *>(mSmallBuffer));
        if (consumedSize < static_cast<std::size_t>(mDecomissionThreshold))
        {
            if (requestedSize > small_buffer_size)
            {
                return errc::buffer_size_exceeded;
            }
            std::memcpy(chunkPart.data(),
                        static_cast<std::byte *>(mSmallBuffer), consumedSize);

            reset(static_cast<std::byte *>(mSmallBuffer), small_buffer_size);
            mDecomissionThreshold -= static_cast<std::int8_t>(consumedSize);
            return oc::success();
        }

        std::memcpy(chunkPart.data(), static_cast<std::byte *>(mSmallBuffer),
                    chunkPart.size());

        DPLX_TRY(acquire_next_chunk());

        auto const overlap = consumedSize
                           - static_cast<std::size_t>(mDecomissionThreshold);
        std::memcpy(
                mCurrentChunk.data(),
                static_cast<std::byte *>(mSmallBuffer)
                        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                        + mDecomissionThreshold,
                overlap);

        mCurrentChunk = mCurrentChunk.subspan(overlap);
        mDecomissionThreshold = -1;
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

            DPLX_TRY(acquire_next_chunk());

            auto const overlap = small_buffer_size
                               - static_cast<unsigned>(mDecomissionThreshold);
            std::memcpy(
                    mCurrentChunk.data(),
                    static_cast<std::byte *>(mSmallBuffer)
                            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                            + chunkPart.size(),
                    overlap);
            mCurrentChunk = mCurrentChunk.subspan(overlap);
            mDecomissionThreshold = -1;
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
        } while (writeAmount != 0);
        reset(mCurrentChunk.data(), mCurrentChunk.size());
        return oc::success();
    }
};

} // namespace dplx::dp::legacy
