
// Copyright Henrik Steffen Gaßmann 2020.
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>
#include <cstring>

#include <concepts>
#include <memory>
#include <new>
#include <span>
#include <type_traits>
#include <utility>

#include <dplx/dp/disappointment.hpp>

namespace dplx::dp
{

template <typename T>
class basic_byte_buffer_view
{
    T *mWindowBegin;
    int mWindowSize;
    int mAllocationSize;

public:
    using element_type = T;
    using value_type = std::remove_cv_t<T>;

    using pointer = T *;
    using const_pointer = T const *;
    using reference = T &;
    using const_reference = T const &;

    using size_type = int;
    using difference_type = int;

    explicit constexpr basic_byte_buffer_view() noexcept
        : mWindowBegin{}
        , mWindowSize{}
        , mAllocationSize{}
    {
    }
    explicit constexpr basic_byte_buffer_view(pointer memory,
                                              size_type allocationSize,
                                              difference_type consumed) noexcept
        : mWindowBegin(memory + consumed)
        , mWindowSize(allocationSize - consumed)
        , mAllocationSize(allocationSize)
    {
    }

    // clang-format off
    template <typename U, std::size_t Extent>
    requires(std::convertible_to<U (*)[], T (*)[]>)
    explicit constexpr basic_byte_buffer_view(std::span<U, Extent> const &memory)
        // clang-format on
        : basic_byte_buffer_view(
                memory.data(), static_cast<int>(memory.size_bytes()), 0)
    {
    }
    // clang-format off
    template <typename U>
    requires(std::convertible_to<U (*)[], T (*)[]>)
    explicit constexpr basic_byte_buffer_view(
        basic_byte_buffer_view<U> const &other)
        // clang-format on
        : mWindowBegin(other.remaining_begin())
        , mWindowSize(static_cast<int>(other.remaining_size()))
        , mAllocationSize(static_cast<int>(other.buffer_size()))
    {
    }

    friend inline void swap(basic_byte_buffer_view &lhs,
                            basic_byte_buffer_view &rhs) noexcept
    {
        using std::swap;
        swap(lhs.mWindowBegin, rhs.mWindowBegin);
        swap(lhs.mWindowSize, rhs.mWindowSize);
        swap(lhs.mAllocationSize, rhs.mAllocationSize);
    }

    inline auto consumed_begin() const noexcept -> pointer
    {
        return mWindowBegin + mWindowSize - mAllocationSize;
    }
    inline auto consumed_end() const noexcept -> pointer
    {
        return mWindowBegin;
    }
    inline auto consumed_size() const noexcept -> size_type
    {
        return mAllocationSize - mWindowSize;
    }
    inline auto consumed() const noexcept -> std::span<element_type>
    {
        return {mWindowBegin + mWindowSize - mAllocationSize,
                static_cast<std::size_t>(mAllocationSize - mWindowSize)};
    }

    inline auto remaining_begin() const noexcept -> pointer
    {
        return mWindowBegin;
    }
    inline auto remaining_end() const noexcept -> pointer
    {
        return mWindowBegin + mWindowSize;
    }
    inline auto remaining_size() const noexcept -> size_type
    {
        return mWindowSize;
    }
    inline auto remaining() const noexcept -> std::span<element_type>
    {
        return {mWindowBegin, static_cast<std::size_t>(mWindowSize)};
    }

    inline auto buffer_size() const noexcept -> size_type
    {
        return mAllocationSize;
    }

    inline void reset() noexcept
    {
        mWindowBegin += mWindowSize - mAllocationSize;
        mWindowSize = mAllocationSize;
    }

    inline auto consume(difference_type const amount) noexcept -> pointer
    {
        mWindowSize -= amount;
        return std::exchange(mWindowBegin, mWindowBegin + amount);
    }
    inline void move_consumer(difference_type const amount) noexcept
    {
        mWindowBegin += amount;
        mWindowSize -= amount;
    }

    inline void move_consumer_to(difference_type const absoluteOffset) noexcept
    {
        auto const newSize = mAllocationSize - absoluteOffset;
        mWindowBegin += mWindowSize - newSize;
        mWindowSize = newSize;
    }
};

using byte_buffer_view = basic_byte_buffer_view<std::byte>;
using const_byte_buffer_view = basic_byte_buffer_view<std::byte const>;

template <typename Allocator>
    requires std::is_same_v<typename Allocator::value_type, std::byte>
class byte_buffer final
{
public:
    using allocator_type = Allocator;
    using allocator_traits = std::allocator_traits<allocator_type>;

private:
    std::span<std::byte> mBuffer;
    allocator_type mAllocator;

public:
    ~byte_buffer() noexcept
    {
        if (mBuffer.data() != nullptr)
        {
            allocator_traits::deallocate(mAllocator, mBuffer.data(),
                                         mBuffer.size());
        }
    }

    explicit byte_buffer() noexcept
        : mBuffer()
        , mAllocator()
    {
    }

    byte_buffer(byte_buffer const &) = delete;
    auto operator=(byte_buffer const &) -> byte_buffer & = delete;

    explicit byte_buffer(byte_buffer &&other) noexcept
        : mBuffer(std::exchange(other.mBuffer, {}))
        , mAllocator(std::move(other.mAllocator))
    {
    }
    auto operator=(byte_buffer &&other) noexcept -> byte_buffer &
    {
        mBuffer = std::exchange(other.mBuffer, {});
        if constexpr (allocator_traits::propagate_on_container_move_assignment::
                              value)
        {
            mAllocator = std::move(other.mAllocator);
        }

        return *this;
    }

    friend inline void swap(byte_buffer &lhs, byte_buffer &rhs) noexcept
    {
        using std::swap;
        swap(lhs.mBuffer, rhs.mBuffer);
        swap(lhs.mAllocator, rhs.mAllocator);
    }

    explicit byte_buffer(allocator_type bufferAllocator)
        : mBuffer()
        , mAllocator(std::move(bufferAllocator))
    {
    }

    auto as_buffer_view() const noexcept -> byte_buffer_view
    {
        return byte_buffer_view(mBuffer);
    }
    auto as_span() const noexcept -> std::span<std::byte>
    {
        return std::span<std::byte>(mBuffer);
    }

    inline auto resize(int const newSize) noexcept -> result<void>
    {
        if (mBuffer.size() == newSize)
        {
            return dp::success();
        }
        if (mBuffer.data() != nullptr)
        {
            allocator_traits::deallocate(mAllocator, mBuffer.data(),
                                         mBuffer.size());

            mBuffer = std::span<std::byte>();
        }

        return allocate(newSize);
    }
    inline auto grow(int const newSize) noexcept -> result<void>
    {
        if (newSize <= mBuffer.size())
        {
            return errc::bad; // TODO: appropriate error code for invalid
                              // parameter values
        }

        auto const oldMemory = mBuffer.data();
        auto const oldSize = mBuffer.size();

        auto allocRx = allocate(newSize);
        if (allocRx)
        {
            std::memcpy(mBuffer.data(), oldMemory, oldSize);

            allocator_traits::deallocate(mAllocator, oldMemory, oldSize);
        }
        return allocRx;
    }

private:
    inline auto allocate(int const bufferSize) noexcept -> result<void>
    {
        try
        {
            auto const bufferSizeT = static_cast<std::size_t>(bufferSize);
            auto const memory
                    = allocator_traits::allocate(mAllocator, bufferSizeT);

            mBuffer = std::span<std::byte>(memory, bufferSizeT);
            return dp::success();
        }
        catch (std::bad_alloc const &)
        {
            return errc::not_enough_memory;
        }
    }
};

} // namespace dplx::dp
