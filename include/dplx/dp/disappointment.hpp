
// Copyright Henrik Steffen Gaßmann 2020.
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <system_error>
#include <type_traits>

#include <outcome/outcome.hpp>

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
    end_of_stream,
    invalid_additional_information,
    item_type_mismatch,
    item_value_out_of_range,
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

#ifndef DPLX_TRY
#define DPLX_TRY(...) OUTCOME_TRY(__VA_ARGS__)
#endif
