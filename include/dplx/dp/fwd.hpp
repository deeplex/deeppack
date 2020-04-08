
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <dplx/dp/concepts.hpp>

namespace dplx::dp
{
static_assert(CHAR_BIT == 8);

template <output_stream Stream, typename T>
class basic_encoder;

template <typename... TArgs>
class mp_varargs
{
};

struct null_type
{
};
inline constexpr null_type null_value{};

} // namespace dplx::dp
