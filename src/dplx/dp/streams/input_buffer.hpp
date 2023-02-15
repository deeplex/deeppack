
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <type_traits>
#include <utility>

#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/fwd.hpp>

namespace dplx::dp
{

/**
 * The minimum_input_buffer_size is in turn the the maximum value for which
 * `input_buffer::require_input()` must not fail with
 * `errc::buffer_size_exceeded`
 */
inline constexpr unsigned minimum_input_buffer_size = 40U;
inline constexpr unsigned minimum_guaranteed_read_size
        = minimum_input_buffer_size;

} // namespace dplx::dp

namespace dplx::dp
{

class input_buffer
{
    std::byte const *mInputBuffer;
    std::size_t mInputBufferSize;
    std::uint64_t mInputSize;

public:
    using element_type = std::byte const;
    using value_type = std::byte;

    using pointer = std::byte const *;
    using iterator = std::byte const *;

    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

protected:
    constexpr ~input_buffer() noexcept = default;
    constexpr input_buffer() noexcept
        : mInputBuffer(nullptr)
        , mInputBufferSize(0U)
        , mInputSize(0U)
    {
    }
    constexpr input_buffer(input_buffer const &) noexcept = default;
    constexpr auto operator=(input_buffer const &) noexcept
            -> input_buffer & = default;

    constexpr input_buffer(input_buffer &&) noexcept = default;
    constexpr auto operator=(input_buffer &&) noexcept
            -> input_buffer & = default;

    static constexpr struct indefinite_input_size_t
    {
    } indefinite_input_size{};

    explicit input_buffer(std::span<std::byte const> inputBuffer,
                          std::uint64_t const inputSize) noexcept
        : mInputBuffer(inputBuffer.data())
        , mInputBufferSize(inputBuffer.size())
        , mInputSize(inputSize)
    {
    }
    explicit input_buffer(pointer const inputBuffer,
                          size_type const inputBufferSize,
                          std::uint64_t const inputSize) noexcept
        : mInputBuffer(inputBuffer)
        , mInputBufferSize(inputBufferSize)
        , mInputSize(inputSize)
    {
    }
    explicit input_buffer(std::span<std::byte const> inputBuffer,
                          indefinite_input_size_t) noexcept
        : mInputBuffer(inputBuffer.data())
        , mInputBufferSize(inputBuffer.size())
        , mInputSize(UINT64_MAX)
    {
    }
    explicit input_buffer(pointer const inputBuffer,
                          size_type const inputBufferSize,
                          indefinite_input_size_t) noexcept
        : mInputBuffer(inputBuffer)
        , mInputBufferSize(inputBufferSize)
        , mInputSize(UINT64_MAX)
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

    [[nodiscard]] auto input_size() const noexcept -> std::uint64_t
    {
        return mInputSize;
    }

    auto require_input(size_type const requiredSize) noexcept -> result<void>
    {
        result<void> rx = oc::success();
        if (requiredSize > mInputSize)
        {
            rx = errc::end_of_stream;
        }
        else if (mInputBufferSize < requiredSize)
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
        mInputSize -= numBytes;
    }

    auto discard_input(std::uint64_t const amount) noexcept -> result<void>
    {
        if (amount <= mInputBufferSize)
        {
            discard_buffered(amount);
            return oc::success();
        }
        if (amount > mInputSize)
        {
            return errc::end_of_stream;
        }

        mInputSize -= mInputBufferSize;
        auto const remaining = amount - mInputBufferSize;
        reset();
        return do_discard_input(remaining);
    }

    auto bulk_read(std::span<std::byte> destBuffer) noexcept -> result<void>
    {
        return bulk_read(destBuffer.data(), destBuffer.size());
    }
    auto bulk_read(std::byte *dest, std::size_t amount) noexcept -> result<void>
    {
        if (amount == 0U) [[unlikely]]
        {
            return oc::success();
        }
        if (amount > mInputSize)
        {
            return errc::end_of_stream;
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
            mInputSize -= copyAmount;
        }
        if (amount == 0U)
        {
            return oc::success();
        }
        return do_bulk_read(dest, amount);
    }

    auto sync_input() noexcept -> result<void>
    {
        return do_sync_input();
    }

protected:
    void reset() noexcept
    {
        mInputBuffer = nullptr;
        mInputSize -= mInputBufferSize;
        mInputBufferSize = 0;
    }
    void reset(std::span<std::byte const> buffer,
               std::uint64_t inputSize) noexcept
    {
        mInputBuffer = buffer.data();
        mInputBufferSize = buffer.size();
        mInputSize = inputSize;
    }
    void reset(pointer const buffer,
               size_type const size,
               std::uint64_t const inputSize) noexcept
    {
        mInputBuffer = buffer;
        mInputBufferSize = size;
        mInputSize = inputSize;
    }
    void reset(std::span<std::byte const> buffer,
               indefinite_input_size_t) noexcept
    {
        mInputBuffer = buffer.data();
        mInputBufferSize = buffer.size();
        mInputSize = UINT64_MAX;
    }
    void reset(pointer const buffer,
               size_type const size,
               indefinite_input_size_t) noexcept
    {
        mInputBuffer = buffer;
        mInputBufferSize = size;
        mInputSize = UINT64_MAX;
    }

private:
    virtual auto do_require_input(size_type requiredSize) noexcept
            -> result<void>
            = 0;
    virtual auto do_discard_input(size_type amount) noexcept -> result<void>
            = 0;
    virtual auto do_bulk_read(std::byte *dest, std::size_t size) noexcept
            -> result<void>
            = 0;
    virtual auto do_sync_input() noexcept -> result<void>
    {
        return oc::success();
    }
};

} // namespace dplx::dp
