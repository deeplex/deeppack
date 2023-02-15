
// Copyright Henrik Steffen Gaßmann 2023
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>
#include <span>

#include <dplx/dp/cpos/stream.hpp>
#include <dplx/dp/fwd.hpp>
#include <dplx/dp/streams/input_buffer.hpp>

namespace dplx::dp
{

// the class is final and none of its base classes have public destructors
// NOLINTNEXTLINE(cppcoreguidelines-virtual-class-destructor)
class memory_input_stream final : public dp::input_buffer
{
public:
    ~memory_input_stream() noexcept = default;
    memory_input_stream() noexcept = default;

    explicit memory_input_stream(std::span<std::byte const> content) noexcept
        : input_buffer(content.data(), content.size(), content.size())
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
                      [[maybe_unused]] std::size_t destSize) noexcept
            -> result<void> override
    {
        return errc::end_of_stream;
    }
};

void tag_invoke(get_input_buffer_fn,
                std::span<std::byte const> const &) noexcept
        = delete;
inline auto tag_invoke(get_input_buffer_fn,
                       std::span<std::byte const> &&content) noexcept
        -> memory_input_stream
{
    return memory_input_stream(content);
}
void tag_invoke(get_input_buffer_fn, std::span<std::byte> const &) noexcept
        = delete;
inline auto tag_invoke(get_input_buffer_fn,
                       std::span<std::byte> &&content) noexcept
        -> memory_input_stream
{
    return memory_input_stream(content);
}

} // namespace dplx::dp
