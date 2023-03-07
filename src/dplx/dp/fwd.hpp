
// Copyright Henrik Steffen Gaßmann 2020-2023
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <climits>
#include <cstdint>
#include <type_traits>

#include <dplx/dp/disappointment.hpp>

namespace dplx::dp
{

static_assert(CHAR_BIT == 8); // NOLINT(cppcoreguidelines-avoid-magic-numbers)

class input_buffer;
class output_buffer;

struct emit_context;
struct parse_context;

class no_codec_available
{
};

template <typename T>
class codec final : public no_codec_available
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

// auto tuple
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

// auto object
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

// layout descriptor support
namespace dplx::dp
{

using version_type = std::uint32_t;
inline constexpr version_type null_def_version = 0xffff'ffffU;

} // namespace dplx::dp

#pragma endregion
