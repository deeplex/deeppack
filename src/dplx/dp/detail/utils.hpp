
// Copyright Henrik Steffen Gaßmann 2020.
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <bit>
#include <cstddef>
#include <limits>

#include <boost/endian/conversion.hpp>

#include <dplx/cncr/utils.hpp>

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

template <std::size_t N>
struct byte_bag
{
    std::byte bytes[N];
};

#if __cpp_lib_byteswap < 202110L
inline
#endif
        namespace cpp20
{
template <typename T>
DPLX_ATTR_FORCE_INLINE void store(std::byte *dest, T value)
{
    if constexpr (std::endian::native == std::endian::little)
    {
        value = boost::endian::endian_reverse<T>(value);
    }

    // std::memcpy(dest, &value, sizeof(T));
    alignas(T) auto raw = std::bit_cast<byte_bag<sizeof(T)>>(value);
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    for (auto src = static_cast<std::byte *>(raw.bytes),
              srcEnd = static_cast<std::byte *>(raw.bytes) + sizeof(T);
         src != srcEnd; ++src, ++dest)
    {
        *dest = *src;
    }
    // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
}

template <typename T>
DPLX_ATTR_FORCE_INLINE auto load(std::byte const *src) noexcept -> T
{
    // T value;
    // std::memcpy(&value, src, sizeof(T));
    alignas(T) byte_bag<sizeof(T)> raw;
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    for (auto dest = static_cast<std::byte *>(raw.bytes),
              destEnd = static_cast<std::byte *>(raw.bytes) + sizeof(T);
         dest != destEnd; ++src, ++dest)
    {
        *dest = *src;
    }
    // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)

    if constexpr (std::endian::native == std::endian::little)
    {
        return boost::endian::endian_reverse<T>(std::bit_cast<T>(raw));
    }
    else
    {
        return std::bit_cast<T>(raw);
    }
}

} // namespace cpp20

#if __cpp_lib_byteswap >= 202110L

inline namespace cpp23
{

template <typename T>
DPLX_ATTR_FORCE_INLINE void store(std::byte *const dest, T value)
{
    if constexpr (std::endian::native == std::endian::little)
    {
        value = std::byteswap<T>(value);
    }

    // std::memcpy(dest, &value, sizeof(T));
    alignas(T) auto raw = std::bit_cast<byte_bag<sizeof(T)>>(value);
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    for (auto src = static_cast<std::byte *>(raw.bytes),
              srcEnd = static_cast<std::byte *>(raw.bytes) + sizeof(T);
         src != srcEnd; ++src, ++dest)
    {
        *dest = *src;
    }
    // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
}

template <typename T>
DPLX_ATTR_FORCE_INLINE auto load(std::byte const *src) noexcept -> T
{
    // T value;
    // std::memcpy(&value, src, sizeof(T));
    alignas(T) byte_bag<sizeof(T)> raw;
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    for (auto dest = static_cast<std::byte *>(raw.bytes),
              destEnd = static_cast<std::byte *>(raw.bytes) + sizeof(T);
         dest != destEnd; ++src, ++dest)
    {
        *dest = *src;
    }
    // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)

    if constexpr (std::endian::native == std::endian::little)
    {
        return std::byteswap<T>(std::bit_cast<T>(raw));
    }
    else
    {
        return std::bit_cast<T>(raw);
    }
}

} // namespace cpp23

#endif

template <typename T>
inline constexpr int digits_v = std::numeric_limits<T>::digits;

} // namespace dplx::dp::detail
