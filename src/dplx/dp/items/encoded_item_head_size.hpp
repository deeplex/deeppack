
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cassert>
#include <cstdint>

#include <dplx/cncr/utils.hpp>

#include <dplx/dp/detail/item_size.hpp>
#include <dplx/dp/items/emit_context.hpp>
#include <dplx/dp/items/type_code.hpp>

namespace dplx::dp
{

constexpr auto encoded_item_head_size(
        type_code const type,
        unsigned long long const additionalInformationValue) noexcept
        -> std::uint64_t
{
    static_assert(sizeof(additionalInformationValue) <= sizeof(std::uint64_t));

    using enum type_code;
    switch (type)
    {
    case special:
        assert(additionalInformationValue
               <= std::numeric_limits<std::uint8_t>::max());
        if (additionalInformationValue
            > std::numeric_limits<std::uint8_t>::max())
        {
            cncr::unreachable();
        }
        [[fallthrough]];

    case posint:
    case negint:
    case binary:
    case text:
    case array:
    case map:
    case tag:
        return detail::var_uint_encoded_size(additionalInformationValue);

    case bool_false:
    case bool_true:
    case null:
    case undefined:
    case special_break:
        assert(additionalInformationValue == 0U);
        return 1U; // NOLINT(cppcoreguidelines-avoid-magic-numbers)

    case float_half:
        assert(additionalInformationValue == 0U);
        return 3U; // NOLINT(cppcoreguidelines-avoid-magic-numbers)
    case float_single:
        assert(additionalInformationValue == 0U);
        return 5U; // NOLINT(cppcoreguidelines-avoid-magic-numbers)
    case float_double:
        assert(additionalInformationValue == 0U);
        return 9U; // NOLINT(cppcoreguidelines-avoid-magic-numbers)

    default:
        assert(false);
        break;
    }

    cncr::unreachable();
}

template <type_code type>
constexpr auto encoded_item_head_size(
        unsigned long long const additionalInformationValue) noexcept
        -> std::uint64_t
{
    static_assert(sizeof(additionalInformationValue) <= sizeof(std::uint64_t));

    using enum type_code;
    switch (type)
    {
    case special:
        assert(additionalInformationValue
               <= std::numeric_limits<std::uint8_t>::max());
        if (additionalInformationValue
            > std::numeric_limits<std::uint8_t>::max())
        {
            cncr::unreachable();
        }
        [[fallthrough]];

    case posint:
    case negint:
    case binary:
    case text:
    case array:
    case map:
    case tag:
        return detail::var_uint_encoded_size(additionalInformationValue);

    case bool_false:
    case bool_true:
    case null:
    case undefined:
    case special_break:
        assert(additionalInformationValue == 0U);
        return 1U; // NOLINT(cppcoreguidelines-avoid-magic-numbers)

    case float_half:
        assert(additionalInformationValue == 0U);
        return 3U; // NOLINT(cppcoreguidelines-avoid-magic-numbers)
    case float_single:
        assert(additionalInformationValue == 0U);
        return 5U; // NOLINT(cppcoreguidelines-avoid-magic-numbers)
    case float_double:
        assert(additionalInformationValue == 0U);
        return 9U; // NOLINT(cppcoreguidelines-avoid-magic-numbers)

    default:
        assert(false);
        break;
    }

    cncr::unreachable();
}

} // namespace dplx::dp
