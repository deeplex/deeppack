
// Copyright Henrik Steffen Gaßmann 2021.
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include <dplx/dp/macros.hpp>

namespace dp_tests
{

namespace
{

class custom_type
{
};

} // namespace

} // namespace dp_tests

DPLX_DP_DECLARE_CODEC_SIMPLE(dp_tests::custom_type);
