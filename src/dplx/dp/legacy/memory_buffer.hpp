
// Copyright Henrik Steffen Gaßmann 2020.
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <concepts>
#include <cstddef>
#include <cstring>
#include <memory>
#include <new>
#include <span>
#include <type_traits>
#include <utility>

#include <dplx/dp/disappointment.hpp>

namespace dplx::dp
{

template <typename T>
class basic_memory_buffer
{
    T *mWindowBegin{};
    unsigned mWindowSize{};
    unsigned mAllocationSize{};

public:
    using element_type = T;
    using value_type = std::remove_cv_t<T>;

    using pointer = T *;
    using const_pointer = T const *;
    using reference = T &;
    using const_reference = T const &;

    using size_type = unsigned;
    using difference_type = int;

    explicit constexpr basic_memory_buffer() noexcept = default;
    explicit constexpr basic_memory_buffer(pointer memory,
                                           size_type allocationSize,
                                           difference_type consumed) noexcept
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        : mWindowBegin(memory + static_cast<size_type>(consumed))
        , mWindowSize(allocationSize - static_cast<size_type>(consumed))
        , mAllocationSize(allocationSize)
    {
    }

    template <typename U, std::size_t Extent>
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays)
        requires(std::convertible_to<U (*)[], T (*)[]>)
    explicit constexpr basic_memory_buffer(std::span<U, Extent> const &memory)
        : basic_memory_buffer(
                  memory.data(), static_cast<size_type>(memory.size_bytes()), 0)
    {
    }
    template <typename U>
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays)
        requires(std::convertible_to<U (*)[], T (*)[]>)
    explicit constexpr basic_memory_buffer(basic_memory_buffer<U> const &other)
        : mWindowBegin(other.remaining_begin())
        , mWindowSize(other.remaining_size())
        , mAllocationSize(other.buffer_size())
    {
    }

    friend inline void swap(basic_memory_buffer &lhs,
                            basic_memory_buffer &rhs) noexcept
    {
        using std::swap;
        swap(lhs.mWindowBegin, rhs.mWindowBegin);
        swap(lhs.mWindowSize, rhs.mWindowSize);
        swap(lhs.mAllocationSize, rhs.mAllocationSize);
    }

    [[nodiscard]] inline auto consumed_begin() const noexcept -> pointer
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        return mWindowBegin + mWindowSize - mAllocationSize;
    }
    [[nodiscard]] inline auto consumed_end() const noexcept -> pointer
    {
        return mWindowBegin;
    }
    [[nodiscard]] inline auto consumed_size() const noexcept -> size_type
    {
        return mAllocationSize - mWindowSize;
    }
    [[nodiscard]] inline auto consumed() const noexcept
            -> std::span<element_type>
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        return {mWindowBegin + mWindowSize - mAllocationSize,
                mAllocationSize - mWindowSize};
    }

    [[nodiscard]] inline auto remaining_begin() const noexcept -> pointer
    {
        return mWindowBegin;
    }
    [[nodiscard]] inline auto remaining_end() const noexcept -> pointer
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        return mWindowBegin + mWindowSize;
    }
    [[nodiscard]] inline auto remaining_size() const noexcept -> size_type
    {
        return mWindowSize;
    }
    [[nodiscard]] inline auto remaining() const noexcept
            -> std::span<element_type>
    {
        return {mWindowBegin, mWindowSize};
    }

    [[nodiscard]] inline auto buffer_size() const noexcept -> size_type
    {
        return mAllocationSize;
    }

    inline void reset() noexcept
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        mWindowBegin += mWindowSize - mAllocationSize;
        mWindowSize = mAllocationSize;
    }

    [[nodiscard]] inline auto consume(difference_type const amount) noexcept
            -> pointer
    {
        mWindowSize -= static_cast<size_type>(amount);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        return std::exchange(mWindowBegin, mWindowBegin + amount);
    }
    inline void move_consumer(difference_type const amount) noexcept
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        mWindowBegin += amount;
        mWindowSize -= static_cast<size_type>(amount);
    }

    inline void move_consumer_to(difference_type const absoluteOffset) noexcept
    {
        auto const newSize = mAllocationSize - absoluteOffset;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        mWindowBegin += mWindowSize - newSize;
        mWindowSize = newSize;
    }
};

using memory_buffer = basic_memory_buffer<std::byte>;
using memory_view = basic_memory_buffer<std::byte const>;

template <typename Allocator>
    requires std::same_as<typename Allocator::value_type, std::byte>
class memory_allocation final
{
public:
    using allocator_type = Allocator;
    using allocator_traits = std::allocator_traits<allocator_type>;

private:
    std::span<std::byte> mBuffer{};
    /*[[no_unique_address]]*/ allocator_type mAllocator{};

public:
    using size_type = unsigned;

    ~memory_allocation() noexcept
    {
        if (mBuffer.data() != nullptr)
        {
            allocator_traits::deallocate(mAllocator, mBuffer.data(),
                                         mBuffer.size());
        }
    }

    explicit memory_allocation() noexcept = default;

    memory_allocation(memory_allocation const &) = delete;
    auto operator=(memory_allocation const &) -> memory_allocation & = delete;

    memory_allocation(memory_allocation &&other) noexcept
        : mBuffer(std::exchange(other.mBuffer, {}))
        , mAllocator(std::move(other.mAllocator))
    {
    }
    auto operator=(memory_allocation &&other) noexcept -> memory_allocation &
    {
        mBuffer = std::exchange(other.mBuffer, {});
        if constexpr (allocator_traits::propagate_on_container_move_assignment::
                              value)
        {
            mAllocator = std::move(other.mAllocator);
        }

        return *this;
    }

    explicit memory_allocation(allocator_type bufferAllocator)
        : mBuffer()
        , mAllocator(std::move(bufferAllocator))
    {
    }

    friend inline void swap(memory_allocation &lhs,
                            memory_allocation &rhs) noexcept
    {
        std::ranges::swap(lhs.mBuffer, rhs.mBuffer);
        std::ranges::swap(lhs.mAllocator, rhs.mAllocator);
    }

    [[nodiscard]] inline auto as_memory_buffer() const noexcept -> memory_buffer
    {
        return dp::memory_buffer(mBuffer);
    }
    [[nodiscard /*, deprecated*/]] auto as_buffer_view() const noexcept
            -> memory_buffer
    {
        return dp::memory_buffer(mBuffer);
    }
    [[nodiscard]] auto as_span() const noexcept -> std::span<std::byte>
    {
        return mBuffer;
    }

    [[nodiscard]] inline auto size() const noexcept -> size_type
    {
        return static_cast<size_type>(mBuffer.size());
    }

    inline auto resize(size_type const newSize) noexcept -> result<void>
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
    inline auto grow(size_type const newSize) noexcept -> result<void>
    {
        if (newSize <= mBuffer.size())
        {
            return errc::bad; // TODO: appropriate error code for invalid
                              // parameter values
        }

        auto const *const oldMemory = mBuffer.data();
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
    inline auto allocate(size_type const bufferSize) noexcept -> result<void>
    {
        try
        {
            auto const memory
                    = allocator_traits::allocate(mAllocator, bufferSize);

            mBuffer = std::span<std::byte>(memory, bufferSize);
            return dp::success();
        }
        catch (std::bad_alloc const &)
        {
            return errc::not_enough_memory;
        }
    }
};

} // namespace dplx::dp
