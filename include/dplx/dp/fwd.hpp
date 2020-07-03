
// Copyright Henrik Steffen Gaßmann 2020.
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <dplx/dp/stream.hpp>

namespace dplx::dp
{

static_assert(CHAR_BIT == 8);

template <output_stream Stream, typename T>
class basic_encoder;

template <input_stream Stream, typename T>
class basic_decoder;

template <typename... TArgs>
class mp_varargs
{
};

struct null_type
{
};
inline constexpr null_type null_value{};

template <typename T>
struct as_value_t
{
    explicit constexpr as_value_t() noexcept = default;
};
template <typename T>
inline constexpr as_value_t<T> as_value;

} // namespace dplx::dp
