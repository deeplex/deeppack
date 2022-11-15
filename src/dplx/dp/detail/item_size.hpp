
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstdint>

#include <dplx/cncr/math_supplement.hpp>

#include <dplx/dp/config.hpp>
#include <dplx/dp/detail/bit.hpp>
#include <dplx/dp/type_code.hpp>

namespace dplx::dp::detail
{

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)

template <typename T>
inline auto var_uint_encoded_byte_power(T const value) noexcept -> int
{
    auto const lastSetBitIndex
            = static_cast<unsigned>(detail::find_last_set_bit(value));
    auto const bytePower = detail::find_last_set_bit(lastSetBitIndex) - 2;

    return bytePower;
}

template <typename T>
inline auto var_uint_encoded_size_ct(T const value) noexcept -> unsigned int
{
    if (value <= inline_value_max)
    {
        return 1U;
    }
    auto const lastSetBitIndex
            = static_cast<unsigned>(detail::find_last_set_bit(value));
    auto const bytePower
            = static_cast<unsigned>(detail::find_last_set_bit(lastSetBitIndex))
            - 2U;

    return 1U + (1U << bytePower);
}

template <typename T>
constexpr auto var_uint_encoded_size_branching(T const value) noexcept
        -> unsigned int
{
    if (value <= inline_value_max)
    {
        return 1U;
    }
    if (value <= 0xFFU)
    {
        return 2U;
    }
    if (value <= 0xFFFFU)
    {
        return 3U;
    }
    if (value <= 0xFFFF'FFFFU)
    {
        return 5U;
    }

    return 9U;
}

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers)

template <cncr::unsigned_integer T>
constexpr auto var_uint_encoded_size(T const value) -> unsigned int
{
    static_assert(sizeof(T) <= sizeof(std::uint64_t));

#if !DPLX_DP_USE_BRANCHING_INTEGER_ENCODER

    if (std::is_constant_evaluated())
    {
        return detail::var_uint_encoded_size_branching(value);
    }

    if constexpr (sizeof(T) <= sizeof(std::uint32_t))
    {
        return var_uint_encoded_size_ct(static_cast<std::uint32_t>(value));
    }
    else
    {
        return var_uint_encoded_size_ct(static_cast<std::uint64_t>(value));
    }

#else

    return detail::var_uint_encoded_size_branching(value);

#endif
}

} // namespace dplx::dp::detail
