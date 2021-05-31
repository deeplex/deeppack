
// Copyright Henrik Steffen Gaßmann 2020-2021.
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <system_error>
#include <type_traits>

#include <outcome/std_result.hpp>
#include <outcome/try.hpp>

namespace dplx::dp
{

namespace oc = OUTCOME_V2_NAMESPACE;

template <typename R, typename EC = std::error_code>
using result = oc::basic_result<R, EC, oc::policy::default_policy<R, EC, void>>;

using oc::failure;
using oc::success;

enum class errc
{
    nothing = 0, // to be removed
    bad = 1,
    end_of_stream,
    invalid_additional_information,
    item_type_mismatch,
    item_value_out_of_range,
    unknown_property,
    too_many_properties,
    item_version_property_missing,
    item_version_mismatch,
    required_object_property_missing,
    not_enough_memory,
    missing_data,
    invalid_indefinite_subitem,
    tuple_size_mismatch,
    duplicate_key,
    oversized_additional_information_coding,
    indefinite_item,
    string_exceeds_size_limit,
};
auto error_category() noexcept -> std::error_category const &;

inline auto make_error_code(errc value) -> std::error_code
{
    return std::error_code(static_cast<int>(value), error_category());
}

} // namespace dplx::dp

namespace std
{

template <>
struct is_error_code_enum<dplx::dp::errc> : std::true_type
{
};

} // namespace std

namespace dplx::dp::detail
{

template <typename B>
concept boolean_testable_impl = std::convertible_to<B, bool>;
template <typename B>
concept boolean_testable = boolean_testable_impl<B> && requires(B &&b)
{
    {
        !static_cast<B &&>(b)
        } -> boolean_testable_impl;
};

template <typename T>
concept tryable = requires(T &&t)
{
    {
        oc::try_operation_has_value(t)
        } -> boolean_testable;
    {
        oc::try_operation_return_as(static_cast<T &&>(t))
        } -> std::convertible_to<result<void>>;
    oc::try_operation_extract_value(static_cast<T &&>(t));
};

template <tryable T>
using result_value_t
        = std::remove_cvref_t<decltype(oc::try_operation_extract_value(
                std::declval<T &&>()))>;

template <typename T, typename R>
concept tryable_result
        = tryable<T> && std::convertible_to<result_value_t<T>, R>;

inline auto try_extract_failure(result<void> in, result<void> &out) -> bool
{
    if (oc::try_operation_has_value(in))
    {
        return false;
    }
    out = oc::try_operation_return_as(std::move(in));
    return true;
}
template <tryable T>
inline auto try_extract_failure(T &&in, result<void> &out) -> bool
{
    if (oc::try_operation_has_value(in))
    {
        return false;
    }
    out = oc::try_operation_return_as(static_cast<T &&>(in));
    return true;
}

} // namespace dplx::dp::detail

#ifndef DPLX_TRY
#define DPLX_TRY(...) OUTCOME_TRY(__VA_ARGS__)
#endif
