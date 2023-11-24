
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

#include <boost/endian/conversion.hpp>

#include <dplx/cncr/utils.hpp>
#include <dplx/predef/compiler.h>

#if defined DPLX_COMP_MSVC_AVAILABLE
#include <intrin.h>
#endif

namespace dplx::dp::detail
{

template <typename T>
inline constexpr int digits_v = std::numeric_limits<T>::digits;

template <std::size_t N>
struct byte_bag
{
    std::byte bytes[N];
};

#if __cpp_lib_byteswap < 202'110L
inline
#endif
        namespace cpp20
{

template <std::integral T>
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

template <std::integral T>
DPLX_ATTR_FORCE_INLINE auto load(std::byte const *src) noexcept -> T
{
    // T value;
    // std::memcpy(&value, src, sizeof(T));
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
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

#if __cpp_lib_byteswap >= 202'110L

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
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
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

template <std::floating_point T>
DPLX_ATTR_FORCE_INLINE void store(std::byte *dest, T value)
{
    static_assert(sizeof(T) == sizeof(std::uint32_t)
                  || sizeof(T) == sizeof(std::uint64_t));
    if constexpr (sizeof(T) == sizeof(std::uint32_t))
    {
        detail::store<std::uint32_t>(dest, std::bit_cast<std::uint32_t>(value));
    }
    else
    {
        detail::store<std::uint64_t>(dest, std::bit_cast<std::uint64_t>(value));
    }
}

template <typename T>
DPLX_ATTR_FORCE_INLINE constexpr auto find_last_set_bit(T value) noexcept -> int
    requires requires { std::countl_zero(value); }
{
    if (std::is_constant_evaluated())
    {
        return (digits_v<T> - 1) - std::countl_zero(value);
    }

#if defined(DPLX_COMP_GCC_AVAILABLE) || defined(DPLX_COMP_CLANG_AVAILABLE)

    if constexpr (sizeof(T) <= sizeof(unsigned int))
    {
        return (digits_v<unsigned int> - 1)
               ^ __builtin_clz(static_cast<unsigned int>(value));
    }
    else if constexpr (sizeof(T) <= sizeof(unsigned long))
    {
        return (digits_v<unsigned long> - 1)
               ^ __builtin_clzl(static_cast<unsigned long>(value));
    }
    else if constexpr (sizeof(T) <= sizeof(unsigned long long))
    {
        return (digits_v<unsigned long long> - 1)
               ^ __builtin_clzll(static_cast<unsigned long long>(value));
    }

#elif defined(DPLX_COMP_MSVC_AVAILABLE)

    if constexpr (sizeof(T) <= sizeof(unsigned long))
    {
        unsigned long result;
        _BitScanReverse(&result, static_cast<unsigned long>(value));
        return static_cast<int>(result);
    }
    else if constexpr (sizeof(T) <= sizeof(unsigned long long))
    {
        unsigned long result;

#if defined(_M_ARM64) || defined(_M_AMD64)

        _BitScanReverse64(&result, static_cast<unsigned long long>(value));
        return static_cast<int>(result);

#else

        static_assert(sizeof(unsigned long) * 2 == sizeof(unsigned long long));

        if (_BitScanReverse(&result, static_cast<unsigned long>(
                                             value >> digits_v<unsigned long>)))
        {
            return static_cast<int>(result + digits_v<unsigned long>);
        }
        _BitScanReverse(&result, static_cast<unsigned long>(value));
        return static_cast<int>(result);
#endif
    }

#endif

    else
    {
        return (digits_v<T> - 1) ^ std::countl_zero(value);
    }
}

template <typename T>
DPLX_ATTR_FORCE_INLINE constexpr auto rotl(T const v, int n) noexcept -> T
    requires requires { std::rotl(v, n); }
{
    return (v << n) | (v >> (digits_v<T> - n));
}

DPLX_ATTR_FORCE_INLINE constexpr auto
byte_swap_u32(std::uint32_t const x) noexcept -> std::uint32_t
{
    if (std::is_constant_evaluated())
    {
        // byte_swap adapted from Boost.Endian
        // Copyright 2019 Peter Dimov
        //
        // Distributed under the Boost Software License, Version 1.0.
        // http://www.boost.org/LICENSE_1_0.txt
        //
        //  -- portable approach suggested by tymofey, with avoidance of
        //     undefined behavior as suggested by Giovanni Piero Deretta,
        //     with a further refinement suggested by Pyry Jahkola.

        // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)

        std::uint32_t const step16 = x << 16 | x >> 16;
        return ((step16 << 8) & 0xff00'ff00) | ((step16 >> 8) & 0x00ff'00ff);

        // NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
    }

    return boost::endian::endian_reverse(x);
}

} // namespace dplx::dp::detail
