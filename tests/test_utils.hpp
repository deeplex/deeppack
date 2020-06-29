
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <array>
#include <cstddef>

#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/type_code.hpp>

#include "boost-test.hpp"

namespace dplx::dp
{

inline auto boost_test_print_type(std::ostream &s, type_code c)
    -> std::ostream &
{
    fmt::print(s, FMT_STRING("{:2x}"), static_cast<std::uint8_t>(c));
    return s;
}

} // namespace dplx::dp

namespace dp_tests
{

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
    std::array<std::byte, N> bs;
    auto last = std::transform(vs.begin(), vs.end(), bs.data(), [](auto v) {
        return static_cast<std::byte>(v);
    });
    std::fill(last, bs.data() + N, fill);
    return bs;
}

template <typename T>
auto make_byte_vector(std::initializer_list<T> vs) noexcept -> std::vector<std::byte>
{
    std::vector<std::byte> bs(vs.size());
    std::transform(vs.begin(), vs.end(), bs.begin(), [](auto v) {
        return static_cast<std::byte>(v);
    });
    return bs;
}

template <typename R>
inline auto check_result(dplx::dp::result<R> const &rx)
    -> boost::test_tools::predicate_result
{
    bool const succeeded = !rx.has_failure();
    boost::test_tools::predicate_result prx{succeeded};
    if (!succeeded)
    {
        prx.message() << rx.assume_error();
    }
    return prx;
}

#define DPLX_TEST_RESULT(...) BOOST_TEST((::dp_tests::check_result((__VA_ARGS__))))
#define DPLX_REQUIRE_RESULT(...)                                               \
    BOOST_TEST_REQUIRE((::dp_tests::check_result((__VA_ARGS__))))

} // namespace dp_tests
