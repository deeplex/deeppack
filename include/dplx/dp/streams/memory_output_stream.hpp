
// Copyright Henrik Steffen Gaßmann 2020.
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <dplx/dp/memory_buffer.hpp>
#include <dplx/dp/stream.hpp>

namespace dplx::dp
{

inline auto
tag_invoke(write_fn, memory_buffer &self, std::size_t const size) noexcept
        -> result<std::span<std::byte>>
{
    if (self.remaining_size() < size)
    {
        return errc::end_of_stream;
    }
    return std::span<std::byte>(self.consume(static_cast<int>(size)), size);
}
inline auto tag_invoke(write_fn,
                       memory_buffer &self,
                       std::byte const *data,
                       std::size_t const size) noexcept -> result<void>
{
    if (self.remaining_size() < size)
    {
        return errc::end_of_stream;
    }
    std::memcpy(self.consume(static_cast<int>(size)), data, size);

    return success();
}

inline auto tag_invoke(commit_fn,
                       memory_buffer &self,
                       std::span<std::byte> const writeProxy,
                       std::size_t const actualSize) noexcept -> result<void>
{
    self.move_consumer(static_cast<int>(actualSize - writeProxy.size()));
    return success();
}

} // namespace dplx::dp
