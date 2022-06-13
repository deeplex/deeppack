
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>
#include <cstdint>

namespace dplx::dp
{

enum class type_code : std::uint8_t
{
    // major types
    posint = 0b000'00000,
    negint = 0b001'00000,
    binary = 0b010'00000,
    text = 0b011'00000,
    array = 0b100'00000,
    map = 0b101'00000,
    tag = 0b110'00000,
    special = 0b111'00000,

    // special values
    bool_false = special | 0b10100,
    bool_true = special | 0b10101,
    null = special | 0b10110,      // models a not set state
    undefined = special | 0b10111, // models a value which couldn't be encoded
    special_extended = special | 0b11000,

    float_half = special | 0b11001,
    float_single = special | 0b11010,
    float_double = special | 0b11011,

    special_break = special | 0b11111, // terminates indefinite sequences
};

inline constexpr auto operator==(std::byte lhs, type_code rhs) -> bool
{
    return static_cast<std::uint8_t>(lhs) == static_cast<std::uint8_t>(rhs);
}

inline constexpr auto to_byte(type_code v) -> std::byte
{
    return std::byte{static_cast<std::uint8_t>(v)};
}

} // namespace dplx::dp

namespace dplx::dp::detail
{

inline constexpr int var_uint_max_size = 9;
inline constexpr unsigned int inline_value_max = 23;

} // namespace dplx::dp::detail
