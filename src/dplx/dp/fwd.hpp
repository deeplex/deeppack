
// Copyright Henrik Steffen Gaßmann 2020.
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <climits>
#include <cstdint>

#include <dplx/dp/stream.hpp>

namespace dplx::dp
{

class output_buffer;

struct emit_context;

template <typename T>
class codec;

} // namespace dplx::dp

namespace dplx::dp
{

static_assert(CHAR_BIT == 8); // NOLINT(cppcoreguidelines-avoid-magic-numbers)

template <typename T, input_stream Stream>
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

#pragma region auto tuple

namespace dplx::dp
{

template <auto... Properties>
struct tuple_def;

template <typename>
struct is_tuple_def : std::false_type
{
};
template <auto... Properties>
struct is_tuple_def<tuple_def<Properties...>> : std::true_type
{
};

template <typename T>
concept tuple_definition = is_tuple_def<T>::value;

template <typename T>
concept is_tuple_def_v = tuple_definition<T>;

} // namespace dplx::dp

#pragma endregion

#pragma region auto object

namespace dplx::dp
{

template <auto... Properties>
struct object_def;

template <typename>
struct is_object_def : std::false_type
{
};
template <auto... Properties>
struct is_object_def<object_def<Properties...>> : std::true_type
{
};

template <typename T>
concept object_definition = is_object_def<T>::value;

template <typename T>
concept is_object_def_v = object_definition<T>;

} // namespace dplx::dp

#pragma endregion

#pragma region layout descriptor support

namespace dplx::dp
{

using version_type = std::uint32_t;
inline constexpr version_type null_def_version = 0xffff'ffffU;

} // namespace dplx::dp

#pragma endregion
