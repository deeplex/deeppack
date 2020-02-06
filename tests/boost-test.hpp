
// Copyright Henrik Steffen Gaßmann 2019
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <boost/predef.h>

#if defined BOOST_COMP_GNUC_AVAILABLE
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#if defined BOOST_COMP_MSVC_AVAILABLE
#pragma warning(push, 3)
#pragma warning(disable : 4702)
#endif

#include <boost/test/unit_test.hpp>

#include <boost/test/data/monomorphic.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/parameterized_test.hpp>

#if defined BOOST_COMP_MSVC_AVAILABLE
#pragma warning(pop)
#endif

#if defined BOOST_COMP_GNUC_AVAILABLE
#pragma GCC diagnostic pop
#endif
