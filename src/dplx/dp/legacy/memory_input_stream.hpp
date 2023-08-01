
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

#include <dplx/dp/cpos/stream.hpp>
#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/legacy/memory_buffer.hpp>
#include <dplx/dp/streams/input_buffer.hpp>

namespace dplx::dp
{

namespace detail
{

template <typename T>
// NOLINTNEXTLINE(cppcoreguidelines-virtual-class-destructor)
class legacy_memory_input_stream final : public input_buffer
{
    basic_memory_buffer<T> &mImpl;

public:
    ~legacy_memory_input_stream() noexcept = default;
    explicit legacy_memory_input_stream(basic_memory_buffer<T> &impl) noexcept
        : input_buffer(impl.remaining_begin(),
                       impl.remaining_size(),
                       impl.remaining_size())
        , mImpl(impl)
    {
    }

private:
    auto do_require_input([[maybe_unused]] size_type requiredSize) noexcept
            -> result<void> override
    {
        return errc::end_of_stream;
    }
    auto do_discard_input([[maybe_unused]] size_type amount) noexcept
            -> result<void> override
    {
        return errc::end_of_stream;
    }
    auto do_bulk_read([[maybe_unused]] std::byte *dest,
                      [[maybe_unused]] std::size_t size) noexcept
            -> result<void> override
    {
        return errc::end_of_stream;
    }
    auto do_sync_input() noexcept -> result<void> override
    {
        auto const writtenSize = data() - mImpl.remaining_begin();
        mImpl.move_consumer(
                static_cast<typename basic_memory_buffer<T>::difference_type>(
                        writtenSize));
        return outcome::success();
    }
};

} // namespace detail

template <typename T>
    requires std::is_same_v<std::byte, std::remove_const_t<T>>
inline auto tag_invoke(get_input_buffer_fn,
                       basic_memory_buffer<T> &view) noexcept
        -> detail::legacy_memory_input_stream<T>
{
    return detail::legacy_memory_input_stream<T>(view);
}

} // namespace dplx::dp
