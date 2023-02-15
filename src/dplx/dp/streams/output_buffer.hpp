
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>
#include <cstring>
#include <span>
#include <type_traits>
#include <utility>

#include <dplx/dp/disappointment.hpp>

namespace dplx::dp
{

/**
 * The minimum_output_buffer_size is in turn the the maximum value for which
 * `output_buffer::ensure_size()` must not fail with
 * `errc::buffer_size_exceeded`
 */
inline constexpr unsigned minimum_output_buffer_size = 40U;
inline constexpr unsigned minimum_guaranteed_write_size
        = minimum_output_buffer_size;

} // namespace dplx::dp

namespace dplx::dp
{

class output_buffer
{
    std::byte *mOutputBuffer;
    std::size_t mOutputBufferSize;

public:
    using element_type = std::byte;
    using value_type = std::byte;

    using pointer = std::byte *;
    using iterator = std::byte *;

    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

protected:
    constexpr ~output_buffer() noexcept = default;
    constexpr output_buffer() noexcept
        : mOutputBuffer(nullptr)
        , mOutputBufferSize(0U)
    {
    }
    constexpr output_buffer(output_buffer const &) noexcept = default;
    constexpr auto operator=(output_buffer const &) noexcept
            -> output_buffer & = default;

public:
    output_buffer(output_buffer &&) noexcept = delete;
    auto operator=(output_buffer &&) noexcept -> output_buffer & = delete;

protected:
    constexpr explicit output_buffer(std::span<std::byte> const buffer) noexcept
        : mOutputBuffer(buffer.data())
        , mOutputBufferSize(buffer.size())
    {
    }
    constexpr explicit output_buffer(std::byte *const bufferData,
                                     size_type const bufferSize) noexcept
        : mOutputBuffer(bufferData)
        , mOutputBufferSize(bufferSize)
    {
    }

public:
    [[nodiscard]] constexpr auto begin() noexcept -> iterator
    {
        return mOutputBuffer;
    }
    [[nodiscard]] constexpr auto end() noexcept -> iterator
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        return mOutputBuffer + mOutputBufferSize;
    }

    [[nodiscard]] constexpr auto data() noexcept -> pointer
    {
        return mOutputBuffer;
    }
    [[nodiscard]] constexpr auto size() const noexcept -> size_type
    {
        return mOutputBufferSize;
    }
    [[nodiscard]] constexpr auto empty() const noexcept -> bool
    {
        return mOutputBufferSize == 0U;
    }

    auto ensure_size(size_type const requestedSize) noexcept -> result<void>
    {
        result<void> rx = oc::success();
        if (mOutputBufferSize < requestedSize)
        {
            rx = do_grow(requestedSize);
        }
        return rx;
    }

    void commit_written(size_type const numBytes) noexcept
    {
        // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        mOutputBuffer += numBytes;
        mOutputBufferSize -= numBytes;
        // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }

    auto bulk_write(std::span<std::byte const> const buffer) noexcept
            -> result<void>
    {
        return bulk_write(buffer.data(), buffer.size());
    }
    auto bulk_write(std::byte const *bytes, std::size_t bytesSize) noexcept
            -> result<void>
    {
        if (bytesSize == 0U) [[unlikely]]
        {
            return oc::success();
        }
        if (mOutputBufferSize > 0U) [[likely]]
        {
            auto const copyAmount = std::min(bytesSize, mOutputBufferSize);
            std::memcpy(mOutputBuffer, bytes, copyAmount);
            // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            bytes += copyAmount;
            bytesSize -= copyAmount;
            mOutputBuffer += copyAmount;
            mOutputBufferSize -= copyAmount;
            // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        }
        if (bytesSize == 0U)
        {
            return oc::success();
        }
        return do_bulk_write(bytes, bytesSize);
    }

    auto sync_output() noexcept -> result<void>
    {
        return do_sync_output();
    }

protected:
    constexpr void reset() noexcept
    {
        mOutputBuffer = nullptr;
        mOutputBufferSize = 0;
    }
    constexpr void reset(std::span<std::byte> const buffer) noexcept
    {
        mOutputBuffer = buffer.data();
        mOutputBufferSize = buffer.size();
    }
    constexpr void reset(std::byte *const buffer,
                         size_type const bufferSize) noexcept
    {
        mOutputBuffer = buffer;
        mOutputBufferSize = bufferSize;
    }

private:
    virtual auto do_grow(size_type requestedSize) noexcept -> result<void> = 0;
    virtual auto do_bulk_write(std::byte const *src,
                               std::size_t srcSize) noexcept -> result<void>
            = 0;
    virtual auto do_sync_output() noexcept -> result<void>
    {
        return oc::success();
    }
};

} // namespace dplx::dp
