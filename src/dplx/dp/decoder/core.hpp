
// Copyright Henrik Steffen Gaßmann 2020.
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include <limits>
#include <ranges>
#include <type_traits>

#include <dplx/cncr/math_supplement.hpp>

#include <dplx/dp/concepts.hpp>
#include <dplx/dp/detail/type_utils.hpp>
#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/fwd.hpp>
#include <dplx/dp/item_parser.hpp>

namespace dplx::dp
{

// volatile types are not supported.
template <typename T, input_stream Stream>
class basic_decoder<volatile T, Stream>;
template <typename T, input_stream Stream>
class basic_decoder<volatile T const, Stream>;

template <cncr::integer T, input_stream Stream>
class basic_decoder<T, Stream>
{
    using parse = item_parser<Stream>;

public:
    auto operator()(Stream &inStream, T &dest) const -> result<void>
    {
        DPLX_TRY(dest, parse::template integer<T>(inStream));
        return oc::success();
    }
};

template <cncr::iec559_floating_point T, input_stream Stream>
class basic_decoder<T, Stream>
{
    static_assert(std::numeric_limits<T>::is_iec559);
    // NOLINTNEXTLINE(readability-magic-numbers)
    static_assert(sizeof(T) == 8 || sizeof(T) == 4);

    using parse = item_parser<Stream>;

public:
    auto operator()(Stream &inStream, T &dest) const -> result<void>
    {
        if constexpr (sizeof(T) == 4)
        {
            DPLX_TRY(dest, parse::float_single(inStream));
            return oc::success();
        }
        else
        {
            DPLX_TRY(dest, parse::float_double(inStream));
            return oc::success();
        }
    }
};

template <input_stream Stream>
class basic_decoder<bool, Stream>
{
    using parse = item_parser<Stream>;

public:
    auto operator()(Stream &inStream, bool &dest) const -> result<void>
    {
        DPLX_TRY(dest, parse::boolean(inStream));
        return oc::success();
    }
};

template <codable_enum Enum, input_stream Stream>
class basic_decoder<Enum, Stream>
{
    using parse = item_parser<Stream>;
    using underlying_type = std::underlying_type_t<Enum>;

public:
    using value_type = Enum;

    auto operator()(Stream &inStream, Enum &value) const -> result<void>
    {
        DPLX_TRY(auto bitRepresentation,
                 parse::template integer<underlying_type>(inStream));

        value = static_cast<Enum>(bitRepresentation);
        return oc::success();
    }
};

} // namespace dplx::dp
