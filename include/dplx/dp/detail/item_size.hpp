
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstdint>

#include <dplx/dp/detail/bit.hpp>
#include <dplx/dp/type_code.hpp>

namespace dplx::dp::detail
{

template <typename T>
inline auto var_uint_encoded_byte_power(T const value) noexcept -> int
{
    int const lastSetBitIndex = detail::find_last_set_bit(value);
    int const bytePower = detail::find_last_set_bit(lastSetBitIndex) - 2;

    return bytePower;
}

template <typename T>
inline auto var_uint_encoded_size_ct(T const value) noexcept -> unsigned int
{
    if (value <= inline_value_max)
    {
        return 1u;
    }
    unsigned int const lastSetBitIndex = detail::find_last_set_bit(value);
    unsigned int const bytePower
            = detail::find_last_set_bit(lastSetBitIndex) - 2;

    return 1u + (1u << bytePower);
}

template <typename T>
constexpr auto var_uint_encoded_size_branching(T const value) noexcept
        -> unsigned int
{
    if (value <= inline_value_max)
    {
        return 1u;
    }
    if (value <= 0xff)
    {
        return 2u;
    }
    if (value <= 0xffff)
    {
        return 3u;
    }
    if (value <= 0xffff'ffff)
    {
        return 5u;
    }
    else
    {
        return 9u;
    }
}

template <unsigned_integer T>
constexpr auto var_uint_encoded_size(T const value) -> unsigned int
{
    static_assert(sizeof(T) <= sizeof(std::uint64_t));

#if !DEEPPACK_USE_BRANCHING_INTEGER_ENCODER

    if (std::is_constant_evaluated())
    {
        return var_uint_encoded_size_branching(value);
    }

    if constexpr (sizeof(T) <= 4)
    {
        return var_uint_encoded_size_ct(static_cast<std::uint32_t>(value));
    }
    else
    {
        return var_uint_encoded_size_ct(static_cast<std::uint64_t>(value));
    }

#else

    return var_uint_encoded_size_branching(value);

#endif
}

} // namespace dplx::dp::detail
