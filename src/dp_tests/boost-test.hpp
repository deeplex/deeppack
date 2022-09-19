
// Copyright Henrik Steffen Gaßmann 2019
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/ranges.h>

#include <dplx/predef/compiler.h>

namespace boost::test_tools::tt_detail::impl
{
inline auto boost_test_print_type(std::ostream &s, std::byte b)
        -> std::ostream &
{
    fmt::print(s, FMT_STRING("{:#04x}"), b);
    return s;
}
template <std::size_t N>
inline auto boost_test_print_type(std::ostream &s,
                                  std::array<std::byte, N> const &arr)
        -> std::ostream &
{
    std::span<std::uint8_t const, N> ui8view(
            reinterpret_cast<std::uint8_t const *>(arr.data()), N);
    fmt::print(s, "{:#04x}", fmt::join(ui8view, ", "));
    return s;
}
template <typename Rep, typename Period>
inline auto boost_test_print_type(std::ostream &s,
                                  std::chrono::duration<Rep, Period> const &v)
        -> std::ostream &
{
    fmt::print(s, "{}", v);
    return s;
}
} // namespace boost::test_tools::tt_detail::impl

#if defined DPLX_COMP_GNUC_AVAILABLE
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#if defined DPLX_COMP_MSVC_AVAILABLE
#pragma warning(push, 3)
#pragma warning(disable : 4702)
#endif

#include <boost/test/data/monomorphic.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>

#if defined DPLX_COMP_MSVC_AVAILABLE
#pragma warning(pop)
#endif

#if defined DPLX_COMP_GNUC_AVAILABLE
#pragma GCC diagnostic pop
#endif
