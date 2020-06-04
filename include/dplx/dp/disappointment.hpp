
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <outcome/outcome.hpp>

namespace dplx::dp
{

namespace oc = OUTCOME_V2_NAMESPACE;

template <typename R, typename EC = std::error_code>
using result = oc::result<R, EC>;

using oc::success;
using oc::failure;

enum class errc
{
};
auto error_category() noexcept -> std::error_category const &;

inline auto make_error_code(errc value) -> std::error_code
{
    return std::error_code(static_cast<int>(value), error_category());
}

}

#ifndef DPLX_TRY
#define DPLX_TRY(...) OUTCOME_TRY(__VA_ARGS__)
#endif
