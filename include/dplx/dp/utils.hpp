
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>

#include <limits>
#include <type_traits>

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

template <class U1, class U2>
inline constexpr int unsigned_digit_distance_v =
    std::numeric_limits<U1>::digits - std::numeric_limits<U2>::digits;

} // namespace dplx::dp::detail
