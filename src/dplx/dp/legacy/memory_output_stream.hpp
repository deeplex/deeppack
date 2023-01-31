
// Copyright Henrik Steffen Gaßmann 2020.
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>

#include <dplx/dp/cpos/stream.hpp>
#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/legacy/memory_buffer.hpp>
#include <dplx/dp/streams/output_buffer.hpp>

namespace dplx::dp
{

namespace detail
{

// NOLINTNEXTLINE(cppcoreguidelines-virtual-class-destructor)
class legacy_memory_output_stream final : public output_buffer
{
    memory_buffer &mImpl;

public:
    ~legacy_memory_output_stream() noexcept = default;
    explicit legacy_memory_output_stream(memory_buffer &impl) noexcept
        : output_buffer(impl.remaining_begin(), impl.remaining_size())
        , mImpl(impl)
    {
    }

private:
    auto do_grow([[maybe_unused]] size_type const requestedSize) noexcept
            -> result<void> override
    {
        return errc::end_of_stream;
    }
    auto do_bulk_write([[maybe_unused]] std::byte const *const src,
                       [[maybe_unused]] std::size_t const size) noexcept
            -> result<void> override
    {
        return errc::end_of_stream;
    }
    auto do_sync_output() noexcept -> result<void> override
    {
        auto const writtenSize = data() - mImpl.remaining_begin();
        mImpl.move_consumer(
                static_cast<memory_buffer::difference_type>(writtenSize));
        return oc::success();
    }
};

} // namespace detail

inline auto tag_invoke(get_output_buffer_fn, memory_buffer &buffer) noexcept
        -> detail::legacy_memory_output_stream
{
    return detail::legacy_memory_output_stream(buffer);
}

} // namespace dplx::dp
