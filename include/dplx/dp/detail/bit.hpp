
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>

#include <bit>
#include <type_traits>

#include <boost/predef/compiler.h>

#include <dplx/dp/detail/utils.hpp>

#if defined BOOST_COMP_MSVC_AVAILABLE
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

#if defined(BOOST_COMP_GCC_AVAILABLE) || defined(BOOST_COMP_CLANG_AVAILABLE)

    if constexpr (sizeof(T) <= sizeof(unsigned int))
    {
        return (digits_v<unsigned int> - 1) ^
               __builtin_clz(static_cast<unsigned int>(value));
    }
    else if constexpr (sizeof(T) <= sizeof(unsigned long))
    {
        return (digits_v<unsigned long> - 1) ^
               __builtin_clzl(static_cast<unsigned long>(value));
    }
    else /*if constexpr (sizeof(T) <= sizeof(unsigned long long))
            see static_assert above */
    {
        return (digits_v<unsigned long long> - 1) ^
               __builtin_clzll(static_cast<unsigned long long>(value));
    }

#elif defined(BOOST_COMP_MSVC_AVAILABLE)

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

        if (_BitScanReverse(
                &result,
                static_cast<unsigned long>(value >> digits_v<unsigned long>)))
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

} // namespace dplx::dp::detail
