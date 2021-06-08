
// Copyright Rene Rivera 2017.
// Copyright Henrik Steffen Gaßmann 2021.
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <boost/predef/version_number.h>

#include <dplx/dp/config.hpp>

// these macros are very similar to those in <boost/predef/other/workaround.h>
// but offer library specific configuration knobs

// guard for bugs which have been resolved with a known (compiler) version
#if DPLX_DP_DISABLE_WORKAROUNDS

#define DPLX_DP_WORKAROUND(symbol, comp, major, minor, patch) (0)

#else

#define DPLX_DP_WORKAROUND(symbol, comp, major, minor, patch)                  \
    ((symbol) >= BOOST_VERSION_NUMBER_AVAILABLE)                               \
            && ((symbol)comp(BOOST_VERSION_NUMBER((major), (minor), (patch))))

#endif

// guard for bugs which have _not_ been resolved known (compiler) version
// i.e. we need to periodically test whether they have been resolved
// after which we can move them in the upper category
#if DPLX_DP_DISABLE_WORKAROUNDS

#define DPLX_DP_WORKAROUND_TESTED_AT(symbol, major, minor, patch) (0)

#elif DPLX_DP_FLAG_OUTDATED_WORKAROUNDS

#define DPLX_DP_WORKAROUND_TESTED_AT(symbol, major, minor, patch)              \
    ((symbol) >= BOOST_VERSION_NUMBER_AVAILABLE)                               \
            && (DPLX_DP_WORKAROUND(symbol, <=, major, minor, patch) ? 1        \
                                                                    : (1 % 0))

#else

#define DPLX_DP_WORKAROUND_TESTED_AT(symbol, major, minor, patch)              \
    ((symbol) >= BOOST_VERSION_NUMBER_AVAILABLE)

#endif
