
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>

#include <limits>
#include <type_traits>
#include <utility>

#include <boost/endian/conversion.hpp>

namespace dplx::dp::detail
{

template <typename T>
void store(std::byte *dest, T value)
{
    boost::endian::endian_store<T, sizeof(T), boost::endian::order::big>(
        reinterpret_cast<unsigned char *>(dest), value);
}

template <typename T>
auto load(std::byte const *src) noexcept -> T
{
    return boost::endian::endian_load<T, sizeof(T), boost::endian::order::big>(
        reinterpret_cast<unsigned char const *>(src));
}

template <typename Target, typename Source>
constexpr auto fits_storage(Source value) -> bool
{
    static_assert(std::is_unsigned_v<Target>);
    static_assert(std::is_unsigned_v<Source>);

    return value <= std::numeric_limits<Target>::max();
}

template <typename T>
inline constexpr int digits_v = std::numeric_limits<T>::digits;

template <typename F, typename T, std::size_t... Is>
constexpr decltype(auto)
apply_simply_impl(F &&f, T &&t, std::index_sequence<Is...>) noexcept(
    noexcept(std::forward<F>(f)(get<Is>(std::forward<T>(t))...)))
{
    return std::forward<F>(f)(get<Is>(std::forward<T>(t))...);
}
// a poor man's std::apply() which however uses unqualified get<I>()
// instead of std::get<I>(). This allows it to cope with custom tuple types.
template <typename F, typename T>
constexpr decltype(auto)
apply_simply(F &&f, T &&t) noexcept(noexcept(dp::detail::apply_simply_impl(
    std::forward<F>(f),
    std::forward<T>(t),
    std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<T>>>())))
{
    return ::dplx::dp::detail::apply_simply_impl<F, T>(
        std::forward<F>(f),
        std::forward<T>(t),
        std::make_index_sequence<
            std::tuple_size_v<std::remove_reference_t<T>>>());
}

} // namespace dplx::dp::detail
