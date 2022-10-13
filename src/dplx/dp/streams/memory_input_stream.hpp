
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

#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/memory_buffer.hpp>
#include <dplx/dp/stream.hpp>

namespace dplx::dp
{

template <typename T>
    requires std::is_same_v<std::byte, std::remove_const_t<T>>
inline auto tag_invoke(cncr::tag_t<dp::available_input_size>,
                       basic_memory_buffer<T> &self) noexcept
        -> dplx::dp::result<std::size_t>
{
    return self.remaining_size();
}
template <typename T>
    requires std::is_same_v<std::byte, std::remove_const_t<T>>
inline auto tag_invoke(cncr::tag_t<dp::read>,
                       basic_memory_buffer<T> &self,
                       std::size_t const amount) noexcept
        -> dplx::dp::result<std::span<std::byte const>>
{
    if (amount > self.remaining_size())
    {
        return errc::end_of_stream;
    }
    return std::span<std::byte const>(self.consume(static_cast<int>(amount)),
                                      amount);
}
template <typename T>
    requires std::is_same_v<std::byte, std::remove_const_t<T>>
inline auto tag_invoke(cncr::tag_t<dp::consume>,
                       basic_memory_buffer<T> &self,
                       std::span<std::byte const> proxy,
                       std::size_t const actualAmount) noexcept
        -> dplx::dp::result<void>
{
    self.move_consumer(-static_cast<int>(proxy.size() - actualAmount));
    return success();
}
template <typename T>
    requires std::is_same_v<std::byte, std::remove_const_t<T>>
inline auto tag_invoke(cncr::tag_t<dp::read>,
                       basic_memory_buffer<T> &self,
                       std::byte *buffer,
                       std::size_t const amount) noexcept
        -> dplx::dp::result<void>
{
    if (amount > self.remaining_size())
    {
        return errc::end_of_stream;
    }

    std::memcpy(buffer, self.consume(static_cast<int>(amount)), amount);
    return success();
}
template <typename T>
    requires std::is_same_v<std::byte, std::remove_const_t<T>>
inline auto tag_invoke(cncr::tag_t<dp::skip_bytes>,
                       basic_memory_buffer<T> &self,
                       std::uint64_t const numBytes) noexcept
        -> dplx::dp::result<void>
{
    if (numBytes > self.remaining_size())
    {
        return errc::end_of_stream;
    }
    self.move_consumer(static_cast<int>(numBytes));
    return oc::success();
}

} // namespace dplx::dp
