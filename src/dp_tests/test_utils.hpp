
// Copyright Henrik Steffen Gaßmann 2020-2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <dplx/predef/compiler.h>

#ifdef DPLX_COMP_GNUC_AVAILABLE
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#endif

#include <array>
#include <chrono>
#include <cstddef>
#include <string>
#include <vector>

#include <fmt/format.h>

#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/type_code.hpp>

#include "boost-test.hpp"

namespace boost::test_tools::tt_detail::impl
{

template <>
inline auto boost_test_print_type<char8_t>(std::ostream &ostr, char8_t const &t)
        -> std::ostream &
{
    fmt::print(ostr, "\\x{:02x}", static_cast<std::uint8_t>(t));
    return ostr;
}
inline auto boost_test_print_type(std::ostream &s, std::u8string const &c)
        -> std::ostream &
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    std::string_view conv{reinterpret_cast<char const *>(c.data()), c.size()};
    s << conv;
    return s;
}

} // namespace boost::test_tools::tt_detail::impl

namespace dplx::dp
{

inline auto boost_test_print_type(std::ostream &s, type_code c)
        -> std::ostream &
{
    fmt::print(s, FMT_STRING("{:2x}"), static_cast<std::uint8_t>(c));
    return s;
}

} // namespace dplx::dp

namespace SYSTEM_ERROR2_NAMESPACE
{

inline auto boost_test_print_type(std::ostream &s,
                                  status_code_domain::string_ref const &ref)
        -> std::ostream &
{
    return s.write(ref.c_str(), std::ssize(ref));
}

template <typename Enum>
inline auto boost_test_print_type(
        std::ostream &s,
        errored_status_code<
                dplx::cncr::data_defined_status_domain_type<Enum>> const &code)
        -> std::ostream &
{
    auto const &cat = code.domain();
    return s << "[domain: " << cat.name().c_str()
             << "; value: " << static_cast<std::intptr_t>(code.value())
             << "; message: " << code.message().c_str() << "]";
}

} // namespace SYSTEM_ERROR2_NAMESPACE

namespace OUTCOME_V2_NAMESPACE
{

template <typename R>
inline auto boost_test_print_type(std::ostream &s,
                                  dplx::dp::result<R> const &rx)
        -> std::ostream &
{
    if (rx.has_value())
    {
        return s << rx.assume_value();
    }
    return boost_test_print_type(s, rx.assume_error());
}

} // namespace OUTCOME_V2_NAMESPACE

namespace dplx::dp
{

inline auto boost_test_print_type(std::ostream &s, errc c) -> std::ostream &
{
    return boost_test_print_type(s, result<void>::error_type(c));
}

} // namespace dplx::dp

namespace dp_tests
{

using namespace dplx;

using byte_span = std::span<std::byte>;

template <typename... Ts>
constexpr auto make_byte_array(Ts... ts) noexcept
        -> std::array<std::byte, sizeof...(Ts)>
{
    static_assert((... && (std::is_integral_v<Ts> || std::is_enum_v<Ts>)));
    return {static_cast<std::byte>(ts)...};
}

template <std::size_t N, typename T>
constexpr auto make_byte_array(std::initializer_list<T> vs,
                               std::byte const fill = std::byte{0xFE}) noexcept
        -> std::array<std::byte, N>
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    std::array<std::byte, N> bs;
    auto *last
            = std::transform(vs.begin(), vs.end(), bs.data(),
                             [](auto v) { return static_cast<std::byte>(v); });
    // std::fill(last, bs.data() + N, fill);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    for (auto *const bsEnd = bs.data() + N; last != bsEnd; ++last)
    {
        *last = fill;
    }
    return bs;
}

template <typename T>
auto make_byte_vector(std::initializer_list<T> vs) noexcept
        -> std::vector<std::byte>
{
    std::vector<std::byte> bs(vs.size());
    std::transform(vs.begin(), vs.end(), bs.begin(),
                   [](auto v) { return static_cast<std::byte>(v); });
    return bs;
}

template <typename R>
inline auto check_result(dp::result<R> const &rx)
        -> boost::test_tools::predicate_result
{
    bool const succeeded = !rx.has_failure();
    boost::test_tools::predicate_result prx{succeeded};
    if (!succeeded)
    {
        boost_test_print_type(prx.message().stream(), rx.assume_error());
    }
    return prx;
}

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DPLX_TEST_RESULT(...)                                                  \
    BOOST_TEST((::dp_tests::check_result((__VA_ARGS__))))
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DPLX_REQUIRE_RESULT(...)                                               \
    BOOST_TEST_REQUIRE((::dp_tests::check_result((__VA_ARGS__))))

} // namespace dp_tests
