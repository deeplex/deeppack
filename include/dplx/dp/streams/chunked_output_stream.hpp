
// Copyright Henrik Steffen Gaßmann 2020.
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>
#include <cstring>

#include <span>

#include <dplx/dp/stream.hpp>

namespace dplx::dp
{

class sbo_write_proxy final
{
    std::byte *mMemory;
    std::size_t mSize;
    std::byte mBuffer[minimum_guaranteed_write_size];

public:
    inline explicit sbo_write_proxy(std::size_t size) noexcept
        : mMemory(nullptr)
        , mSize(size)
    {
    }
    inline explicit sbo_write_proxy(std::span<std::byte> memory) noexcept
        : mMemory(memory.data())
        , mSize(memory.size())
    {
    }

    inline auto uses_small_buffer() const noexcept -> bool
    {
        return mMemory == nullptr;
    }

    inline auto data() noexcept -> std::byte *
    {
        return mMemory != nullptr ? mMemory : mBuffer;
    }
    inline auto size() const noexcept -> std::size_t
    {
        return mSize;
    }

    inline auto begin() noexcept -> std::byte *
    {
        return data();
    }
    inline auto end() noexcept -> std::byte *
    {
        return data() + mSize;
    }
};

template <typename Impl>
class chunked_output_stream_base
{
    std::span<std::byte> mWriteArea;
    std::uint64_t mRemaining;

protected:
    chunked_output_stream_base(std::span<std::byte> initialWriteArea,
                               std::uint64_t remaining) noexcept
        : mWriteArea(initialWriteArea)
        , mRemaining(remaining)
    {
    }

private:
    auto impl() noexcept -> Impl *
    {
        return static_cast<Impl *>(this);
    }

    auto acquire_next_chunk() noexcept -> result<void>
    {
        using byte_span = std::span<std::byte>;
        DPLX_TRY(byte_span const nextChunk, impl()->acquire_next_chunk_impl());

        mWriteArea
                = nextChunk.size() > mRemaining
                        ? nextChunk.first(static_cast<std::size_t>(mRemaining))
                        : nextChunk;

        mRemaining -= mWriteArea.size();

        return success();
    }

    auto write(std::size_t const size) noexcept -> result<sbo_write_proxy>
    {
        if (mWriteArea.size() >= size)
        {
            auto const buffer = mWriteArea.first(size);
            mWriteArea = mWriteArea.subspan(size);
            return sbo_write_proxy(buffer);
        }
        else if (mWriteArea.size() + mRemaining < size)
        {
            return dplx::dp::errc::end_of_stream;
        }
        else if (mWriteArea.size() == 0)
        {
            DPLX_TRY(this->acquire_next_chunk());
            return write(size);
        }

        // sector wrap around
        if (size > dp::minimum_guaranteed_write_size)
        {
            return this->write(dp::minimum_guaranteed_write_size);
        }

        return sbo_write_proxy(size);
    }

    auto commit(sbo_write_proxy &proxy, std::size_t const size) noexcept
            -> result<void>
    {
        if (!proxy.uses_small_buffer())
        {
            auto const diff = proxy.size() - size;
            mWriteArea = std::span<std::byte>(mWriteArea.data() - diff,
                                              mWriteArea.size() + diff);

            return success();
        }

        // write() will only return a reference to the secondary
        // buffer if the write doesn't fit into mWriteArea

        std::span<std::byte> const buffer = proxy;

        auto const firstChunkSize = std::min(buffer.size(), mWriteArea.size());
        std::memcpy(mWriteArea.data(), buffer.data(), firstChunkSize);

        // however, commit might shrink it back into a fitting chunk
        if (buffer.size() < mWriteArea.size())
        {
            mWriteArea = mWriteArea.subspan(firstChunkSize);
            return success();
        }

        auto const secondChunk = buffer.subspan(firstChunkSize);
        DPLX_TRY(this->acquire_next_chunk());

        // secondChunk.size() < minimum_guaranteed_write_size < chunk size
        std::memcpy(mWriteArea.data(), secondChunk.data(), secondChunk.size());

        return success();
    }

    auto write(std::byte const *data, std::size_t const size) noexcept
            -> result<void>
    {
        if (mWriteArea.size() + mRemaining < size)
        {
            return dp::errc::end_of_stream;
        }

        for (std::span<std::byte const> remaining(data, size);
             remaining.size() > 0;)
        {
            auto const chunk = std::min(remaining.size(), mWriteArea.size());

            std::memcpy(mWriteArea.data(), remaining.data(), chunk);

            remaining = remaining.subspan(chunk);
            mWriteArea = mWriteArea.subspan(chunk);

            if (mWriteArea.size() == 0)
            {
                DPLX_TRY(this->acquire_next_chunk());
            }
        }

        return success();
    }

public:
    friend inline auto tag_invoke(tag_t<dp::write>,
                                  chunked_output_stream_base &self,
                                  std::size_t const size) noexcept
            -> result<sbo_write_proxy>
    {
        return self.write(size);
    }

    friend inline auto tag_invoke(tag_t<dp::commit>,
                                  chunked_output_stream_base &stream,
                                  sbo_write_proxy &proxy) noexcept
            -> result<void>
    {
        return stream.commit(proxy, proxy.size());
    }
    friend inline auto tag_invoke(tag_t<dp::commit>,
                                  chunked_output_stream_base &stream,
                                  sbo_write_proxy &proxy,
                                  std::size_t const actualSize) noexcept
            -> result<void>
    {
        return stream.commit(proxy, actualSize);
    }

    friend inline auto tag_invoke(tag_t<dp::write>,
                                  chunked_output_stream_base &self,
                                  std::byte const *data,
                                  std::size_t const size) noexcept
            -> result<void>
    {
        return self.write(data, size);
    }
};

} // namespace dplx::dp
