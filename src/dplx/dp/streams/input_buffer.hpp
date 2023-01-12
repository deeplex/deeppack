
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>
#include <cstring>
#include <type_traits>
#include <utility>

#include <dplx/dp/disappointment.hpp>

namespace dplx::dp
{

class input_buffer
{
    std::byte const *mInputBuffer;
    std::size_t mInputBufferSize;

public:
    using element_type = std::byte const;
    using value_type = std::byte;

    using pointer = std::byte const *;
    using iterator = std::byte const *;

    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

protected:
    ~input_buffer() noexcept = default;
    input_buffer() noexcept
        : mInputBuffer(nullptr)
        , mInputBufferSize(0U)
    {
    }
    input_buffer(input_buffer const &) noexcept = default;
    auto operator=(input_buffer const &) noexcept -> input_buffer & = default;

public:
    input_buffer(input_buffer &&) noexcept = delete;
    auto operator=(input_buffer &&) noexcept -> input_buffer & = delete;

protected:
    explicit input_buffer(pointer writeBuffer, size_type const size) noexcept
        : mInputBuffer(writeBuffer)
        , mInputBufferSize(size)
    {
    }

public:
    [[nodiscard]] auto begin() noexcept -> iterator
    {
        return mInputBuffer;
    }
    [[nodiscard]] auto end() noexcept -> iterator
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        return mInputBuffer + mInputBufferSize;
    }

    [[nodiscard]] auto data() noexcept -> pointer
    {
        return mInputBuffer;
    }
    [[nodiscard]] auto size() const noexcept -> size_type
    {
        return mInputBufferSize;
    }
    [[nodiscard]] auto empty() const noexcept -> bool
    {
        return mInputBufferSize == 0U;
    }

    auto require_input(size_type const requiredSize) noexcept -> result<void>
    {
        result<void> rx = oc::success();
        if (mInputBufferSize < requiredSize)
        {
            rx = do_require_input(requiredSize);
        }
        return rx;
    }

    void discard_buffered(size_type const numBytes) noexcept
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        mInputBuffer += numBytes;
        mInputBufferSize -= numBytes;
    }

    auto discard_input(size_type const amount) noexcept -> result<void>
    {
        if (amount <= mInputBufferSize)
        {
            discard_buffered(amount);
            return oc::success();
        }

        auto const remaining = amount - mInputBufferSize;
        reset();
        return do_discard_input(remaining);
    }

    auto bulk_read(std::byte *dest, std::size_t amount) noexcept -> result<void>
    {
        if (amount == 0U) [[unlikely]]
        {
            return oc::success();
        }
        if (mInputBufferSize > 0U) [[likely]]
        {
            auto const copyAmount = std::min(mInputBufferSize, amount);
            std::memcpy(dest, mInputBuffer, copyAmount);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            dest += copyAmount;
            amount -= copyAmount;
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            mInputBuffer += copyAmount;
            mInputBufferSize -= copyAmount;
        }
        if (amount == 0U)
        {
            return oc::success();
        }
        return do_bulk_read(dest, amount);
    }

protected:
    void reset() noexcept
    {
        mInputBuffer = nullptr;
        mInputBufferSize = 0;
    }
    void reset(pointer const buffer, size_type const size) noexcept
    {
        mInputBuffer = buffer;
        mInputBufferSize = size;
    }

private:
    virtual auto do_require_input(size_type requiredSize) noexcept
            -> result<void> = 0;
    virtual auto do_discard_input(size_type amount) noexcept
            -> result<void> = 0;
    virtual auto do_bulk_read(std::byte *dest, std::size_t size) noexcept
            -> result<void> = 0;
};

} // namespace dplx::dp
