
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <array>
#include <cstddef>
#include <span>

#include <dplx/dp/customization.hpp>

namespace dplx::dp
{

template <typename T, std::size_t N>
inline auto tag_invoke(container_reserve_fn,
                       std::array<T, N> &c,
                       std::size_t size) noexcept -> result<void>
{
    if (size <= c.size())
    {
        return oc::success();
    }
    return errc::not_enough_memory;
}
template <typename T, std::size_t N>
inline auto tag_invoke(container_resize_fn,
                       std::array<T, N> &c,
                       std::size_t size) noexcept -> result<void>
{
    if (size <= c.size())
    {
        return oc::success();
    }
    return errc::not_enough_memory;
}

template <typename T, std::size_t N>
inline auto tag_invoke(container_reserve_fn,
                       std::span<T, N> &c,
                       std::size_t size) noexcept -> result<void>
{
    if (size <= c.size())
    {
        return oc::success();
    }
    return errc::not_enough_memory;
}
template <typename T, std::size_t N>
inline auto tag_invoke(container_resize_fn,
                       std::span<T, N> &c,
                       std::size_t size) noexcept -> result<void>
{
    if (size <= c.size())
    {
        return oc::success();
    }
    return errc::not_enough_memory;
}

} // namespace dplx::dp
