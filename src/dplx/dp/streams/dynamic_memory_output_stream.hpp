
// Copyright Henrik Steffen Gaßmann 2023
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include <boost/container/small_vector.hpp>
#include <boost/container/vector.hpp>

#include <dplx/cncr/math_supplement.hpp>

#include <dplx/dp/cpos/container.hpp>
#include <dplx/dp/fwd.hpp>
#include <dplx/dp/streams/output_buffer.hpp>

namespace dplx::dp
{

template <typename Allocator = std::allocator<std::byte>>
// the class is final and none of its base classes have public destructors
// NOLINTNEXTLINE(cppcoreguidelines-virtual-class-destructor)
class dynamic_memory_output_stream final : public output_buffer
{
    static constexpr std::size_t page_size = 4096U;
    using alloc_traits = std::allocator_traits<Allocator>;

    using buffer_type = std::vector<
            std::byte,
            typename alloc_traits::template rebind_alloc<std::byte>>;

    buffer_type mBuffer{};

public:
    constexpr dynamic_memory_output_stream() noexcept(noexcept(buffer_type()))
            = default;

    explicit dynamic_memory_output_stream(buffer_type buffer) noexcept
        : output_buffer(buffer.data(), buffer.size())
        , mBuffer(std::move(buffer))
    {
    }
    explicit dynamic_memory_output_stream(Allocator const &alloc) noexcept
        : output_buffer()
        , mBuffer(alloc)
    {
    }

    [[nodiscard]] auto written() const & noexcept -> std::span<std::byte const>
    {
        return std::span<std::byte const>(mBuffer).subspan(
                0U, mBuffer.size() - size());
    }
    [[nodiscard]] auto written() && noexcept -> buffer_type
    {
        mBuffer.resize(mBuffer.size() - size());
        output_buffer::reset();
        return std::move(mBuffer);
    }
    [[nodiscard]] auto written_size() const noexcept -> std::size_t
    {
        return mBuffer.size() - size();
    }

private:
    auto do_grow(size_type const requestedSize) noexcept
            -> result<void> override
    try
    {
        auto const offset = static_cast<std::size_t>(data() - mBuffer.data());
        mBuffer.resize(buffer_size_for(mBuffer.size() + requestedSize));
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        output_buffer::reset(mBuffer.data() + offset, mBuffer.size() - offset);
        return outcome::success();
    }
    catch (std::bad_alloc const &)
    {
        return system_error::errc::not_enough_memory;
    }
    auto do_bulk_write(std::byte const *const srcData,
                       std::size_t const srcSize) noexcept
            -> result<void> override
    {
        DPLX_TRY(dynamic_memory_output_stream::do_grow(srcSize));
        std::memcpy(data(), srcData, srcSize);
        commit_written(srcSize);
        return outcome::success();
    }

    [[nodiscard]] static auto buffer_size_for(std::size_t required) noexcept
            -> std::size_t
    {
        constexpr std::size_t overflowThreshold = SIZE_MAX / 3 * 2 - page_size;
        if (required >= overflowThreshold) [[unlikely]]
        {
            return SIZE_MAX;
        }

        auto acc = page_size;
        while (acc < required)
        {
            acc = cncr::round_up_p2(acc * 3 / 2, page_size);
        }
        return acc;
    }
};

template <typename Allocator>
dynamic_memory_output_stream(std::vector<std::byte, Allocator> buffer)
        -> dynamic_memory_output_stream<Allocator>;

extern template class dynamic_memory_output_stream<std::allocator<std::byte>>;

} // namespace dplx::dp
