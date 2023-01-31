
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cassert>
#include <cstdint>

#include <dplx/dp/items/emit_context.hpp>
#include <dplx/dp/items/encoded_item_head_size.hpp>

namespace dplx::dp
{

template <detail::encodable_int T>
[[nodiscard]] constexpr auto item_size_of_integer(emit_context const &,
                                                  T value) noexcept
        -> std::uint64_t
{
    using code_type = detail::encoder_uint_t<T>;
    if constexpr (!std::is_signed_v<T>)
    {
        return dp::encoded_item_head_size<type_code::posint>(
                static_cast<code_type>(value));
    }
    else
    {
        using uvalue_type = std::make_unsigned_t<T>;
        auto const signmask = static_cast<uvalue_type>(
                value >> (detail::digits_v<uvalue_type> - 1U));
        // complement negatives
        uvalue_type const uvalue = signmask ^ static_cast<uvalue_type>(value);

        // posint vs negint encoding has no influence on encoding size
        return dp::encoded_item_head_size<type_code::posint>(
                static_cast<code_type>(uvalue));
    }
}

template <detail::encodable_int T>
[[nodiscard]] constexpr auto item_size_of_binary(emit_context const &,
                                                 T byteSize) noexcept
        -> std::uint64_t
{
    using code_type = detail::encoder_uint_t<T>;
    assert(byteSize >= 0);
    return dp::encoded_item_head_size<type_code::binary>(
                   static_cast<code_type>(byteSize))
         + static_cast<std::uint64_t>(byteSize);
}
template <detail::encodable_int T>
[[nodiscard]] constexpr auto
item_size_of_binary_indefinite(emit_context const &, T byteSize) noexcept
        -> std::uint64_t
{
    assert(byteSize >= 0);
    return 1U + static_cast<std::uint64_t>(byteSize) + 1U;
}

template <detail::encodable_int T>
[[nodiscard]] constexpr auto item_size_of_u8string(emit_context const &,
                                                   T numCodeUnits) noexcept
        -> std::uint64_t
{
    using code_type = detail::encoder_uint_t<T>;
    assert(numCodeUnits >= 0);
    return dp::encoded_item_head_size<type_code::text>(
                   static_cast<code_type>(numCodeUnits))
         + static_cast<std::uint64_t>(numCodeUnits);
}
template <detail::encodable_int T>
[[nodiscard]] constexpr auto
item_size_of_u8string_indefinite(emit_context const &, T numCodeUnits) noexcept
        -> std::uint64_t
{
    assert(numCodeUnits >= 0);
    return 1U + static_cast<std::uint64_t>(numCodeUnits) + 1U;
}

[[nodiscard]] constexpr auto item_size_of_boolean(emit_context const &,
                                                  bool) noexcept
        -> std::uint64_t
{
    return 1U;
}

[[nodiscard]] constexpr auto item_size_of_float_single(emit_context const &,
                                                       float) noexcept
        -> std::uint64_t
{
    return 5U; // NOLINT(cppcoreguidelines-avoid-magic-numbers)
}

[[nodiscard]] constexpr auto item_size_of_float_double(emit_context const &,
                                                       double) noexcept
        -> std::uint64_t
{
    return 9U; // NOLINT(cppcoreguidelines-avoid-magic-numbers)
}

[[nodiscard]] constexpr auto item_size_of_null(emit_context const &) noexcept
        -> std::uint64_t
{
    return 1U;
}

[[nodiscard]] constexpr auto
item_size_of_undefined(emit_context const &) noexcept -> std::uint64_t
{
    return 1U;
}

} // namespace dplx::dp
