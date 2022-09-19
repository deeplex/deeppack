
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <bit>
#include <cstddef>
#include <type_traits>

#include <boost/endian/arithmetic.hpp>

#include <dplx/cncr/math_supplement.hpp>
#include <dplx/predef/compiler.h>

#include <dplx/dp/detail/type_utils.hpp>
#include <dplx/dp/detail/utils.hpp>

#if defined DPLX_COMP_MSVC_AVAILABLE
#include <intrin.h>
#endif

namespace dplx::dp::detail
{

template <typename T>
constexpr auto find_last_set_bit(T value) noexcept -> int
{
    static_assert(std::is_integral_v<T>);
    static_assert(std::is_unsigned_v<T>);
    static_assert(sizeof(T) <= sizeof(unsigned long long));

    // #LangODR to be resolved after MSVC supports __cpp_lib_bitops
#if __cpp_lib_bitops >= 201907L && __cpp_lib_is_constant_evaluated >= 201811L

    if (std::is_constant_evaluated())
    {
        return (digits_v<T> - 1) - std::countl_zero(value);
    }

#endif

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
    else /*if constexpr (sizeof(T) <= sizeof(unsigned long long))
            see static_assert above */
    {
        return (digits_v<unsigned long long> - 1)
             ^ __builtin_clzll(static_cast<unsigned long long>(value));
    }

#elif defined(DPLX_COMP_MSVC_AVAILABLE)

    unsigned long result;
    if constexpr (sizeof(T) <= sizeof(unsigned long))
    {
        _BitScanReverse(&result, static_cast<unsigned long>(value));
        return static_cast<int>(result);
    }
    else if constexpr (sizeof(T) <= sizeof(unsigned long long))
    {
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
        else
        {
            _BitScanReverse(&result, static_cast<unsigned long>(value));
            return static_cast<int>(result);
        }
#endif
    }

#else

    return (digits_v<T> - 1) ^ std::countl_zero(value);

#endif
}

template <cncr::unsigned_integer T>
constexpr auto rotl(T const v, int n) noexcept -> T
{
    return (v << n) | (v >> (digits_v<T> - n));
}

template <cncr::integer T, std::endian order>
constexpr auto load(std::byte const *const src) -> T
{
    // static_assert(order == std::endian::native);
    static_assert(order == std::endian::big || order == std::endian::little);

    if (std::is_constant_evaluated())
    {
        static_assert(sizeof(T) <= 8);
        using uT = std::make_unsigned_t<T>;
        if constexpr (order == std::endian::little)
        {
            uT acc = std::to_integer<uT>(src[0]);
            if constexpr (sizeof(acc) >= 2)
            {
                acc |= std::to_integer<uT>(src[1]) << 8;
            }
            if constexpr (sizeof(acc) >= 4)
            {
                acc |= std::to_integer<uT>(src[2]) << 16
                     | std::to_integer<uT>(src[3]) << 24;
            }
            if constexpr (sizeof(acc) == 8)
            {
                acc |= std::to_integer<uT>(src[4]) << 32
                     | std::to_integer<uT>(src[5]) << 40
                     | std::to_integer<uT>(src[6]) << 48
                     | std::to_integer<uT>(src[7]) << 56;
            }
            return static_cast<T>(acc);
        }
        else
        {
            uT acc = std::to_integer<uT>(src[0]) << 56;
            if constexpr (sizeof(acc) >= 2)
            {
                acc |= std::to_integer<uT>(src[1]) << 48;
            }
            if constexpr (sizeof(acc) >= 4)
            {
                acc |= std::to_integer<uT>(src[2]) << 40
                     | std::to_integer<uT>(src[3]) << 32;
            }
            if constexpr (sizeof(acc) == 8)
            {
                acc |= std::to_integer<uT>(src[4]) << 24
                     | std::to_integer<uT>(src[5]) << 16
                     | std::to_integer<uT>(src[6]) << 8
                     | std::to_integer<uT>(src[7]);
            }
            return static_cast<T>(acc >> (64 - digits_v<uT>));
        }
    }
    else
    {
        constexpr auto boostOrder = order == std::endian::little
                                          ? boost::endian::order::little
                                          : boost::endian::order::big;
        return boost::endian::endian_load<T, sizeof(T), boostOrder>(
                reinterpret_cast<unsigned char const *>(src));
        // T assembled;
        // std::memcpy(&assembled, src, sizeof(assembled));
        // return assembled;
    }
}

template <cncr::integer T, std::endian order>
constexpr auto load_partial(std::byte const *data, int num) -> T
{
    static_assert(sizeof(T) <= 8);
    static_assert(order == std::endian::big || order == std::endian::little);

    using uT = std::make_unsigned_t<T>;
    if constexpr (order == std::endian::little)
    {
        uT assembled = 0;
        switch (num)
        {
        case 7:
            if constexpr (sizeof(assembled) == 8)
            {
                assembled |= std::to_integer<uT>(data[6]) << 48;
            }
            [[fallthrough]];
        case 6:
            if constexpr (sizeof(assembled) == 8)
            {
                assembled |= std::to_integer<uT>(data[5]) << 40;
            }
            [[fallthrough]];
        case 5:
            if constexpr (sizeof(assembled) == 8)
            {
                assembled |= std::to_integer<uT>(data[4]) << 32;
            }
            [[fallthrough]];

        case 4:
            if constexpr (sizeof(assembled) >= 4)
            {
                assembled |= std::to_integer<uT>(data[3]) << 24;
            }
            [[fallthrough]];
        case 3:
            if constexpr (sizeof(assembled) >= 4)
            {
                assembled |= std::to_integer<uT>(data[2]) << 16;
            }
            [[fallthrough]];

        case 2:
            if constexpr (sizeof(assembled) >= 2)
            {
                assembled |= std::to_integer<uT>(data[1]) << 8;
            }
            [[fallthrough]];

        case 1:
            assembled |= std::to_integer<uT>(data[0]);
            [[fallthrough]];

        case 0:
            break;
        }
        return assembled;
    }
    else
    {
    }
} // namespace dplx::dp::detail

constexpr auto byte_swap_u32(std::uint32_t const x) noexcept -> std::uint32_t
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

        std::uint32_t const step16 = x << 16 | x >> 16;
        return ((step16 << 8) & 0xff00ff00) | ((step16 >> 8) & 0x00ff00ff);
    }
    else
    {
        return boost::endian::endian_reverse(x);
    }
}

} // namespace dplx::dp::detail
