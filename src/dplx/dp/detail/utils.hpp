
// Copyright Henrik Steffen Gaßmann 2020.
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

#if __has_cpp_attribute(likely) >= 201803
#define DPLX_ATTR_LIKELY [[likely]]
#else
#define DPLX_ATTR_LIKELY
#endif

#if __has_cpp_attribute(unlikely) >= 201803
#define DPLX_ATTR_UNLIKELY [[unlikely]]
#else
#define DPLX_ATTR_UNLIKELY
#endif

namespace dplx::dp::detail
{

template <typename T>
void store(std::byte *dest, T value)
{
    boost::endian::endian_store<T, sizeof(T), boost::endian::order::big>(
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            reinterpret_cast<unsigned char *>(dest), value);
}

template <typename T>
auto load(std::byte const *src) noexcept -> T
{
    return boost::endian::endian_load<T, sizeof(T), boost::endian::order::big>(
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            reinterpret_cast<unsigned char const *>(src));
}

template <typename T>
inline constexpr int digits_v = std::numeric_limits<T>::digits;

} // namespace dplx::dp::detail
