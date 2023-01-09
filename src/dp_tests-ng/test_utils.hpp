
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <fmt/format.h>

#include <dplx/predef/compiler.h>

#ifdef DPLX_COMP_GNUC_AVAILABLE
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#endif

namespace dplx
{
}

namespace dp_tests
{

using namespace dplx;

namespace detail
{

template <typename T>
concept is_fmt_formattable = fmt::is_formattable<T>::value;

}

} // namespace dp_tests
