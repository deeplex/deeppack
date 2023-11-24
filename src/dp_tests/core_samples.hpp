
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>
#include <limits>
#include <ranges>
#include <utility>

#include <dplx/predef/compiler.h>

#include "dplx/dp/detail/workaround.hpp"
#include "item_sample_ct.hpp"
#include "range_generator.hpp"

namespace dp_tests
{

using namespace std::string_view_literals;

template <typename T>
using limits = std::numeric_limits<T>;

template <typename R, typename T, std::size_t N>
auto integer_samples(item_sample_ct<T> const (&samples)[N])
{
#if DPLX_DP_WORKAROUND_TESTED_AT(DPLX_COMP_CLANG, 15, 0, 0)
    typename iterator_generator<item_sample_ct<R>>::container_type values;
    values.reserve(N);
    for (auto &sample : samples)
    {
        if (std::in_range<R>(sample.value))
        {
            values.push_back(sample.template as<R>());
        }
    }

    return dp_tests::from_range(std::move(values));
#else
    using namespace std::ranges::views;

    return dp_tests::from_range(
            samples | filter([](item_sample_ct<T> const &sample) {
                return std::in_range<R>(sample.value);
            })
            | transform([](item_sample_ct<T> const &sample) {
                  return sample.template as<R>();
              }));
#endif
}

constexpr item_sample_ct<unsigned long long> posint_samples[] = {
  // Appendix A.Examples
        {0x00, 1, {0b000'00000}},
        {0x01, 1, {0b000'00001}},
        {0x0a, 1, {0b000'01010}},
        {0x19, 2, {0x18, 0x19}},
        {0x64, 2, {0x18, 0x64}},
        {0x03e8, 3, {0x19, 0x03, 0xe8}},
        {0x000f'4240, 5, {0x1a, 0x00, 0x0f, 0x42, 0x40}},
        {
         0x0000'00e8'd4a5'1000, 9,
         {0x1b, 0x00, 0x00, 0x00, 0xe8, 0xd4, 0xa5, 0x10, 0x00},
         },
 // boundary test cases
        {0x00, 1, {0b000'00000}},
        {0x17, 1, {0b000'10111}},
        {0x18, 2, {0x18, 0x18}},
        {0x7f, 2, {0x18, 0x7f}},
        {0x80, 2, {0x18, 0x80}},
        {0xff, 2, {0x18, 0xff}},
        {0x0100, 3, {0x19, 0x01, 0x00}},
        {0x7fff, 3, {0x19, 0x7f, 0xff}},
        {0x8000, 3, {0x19, 0x80, 0x00}},
        {0xffff, 3, {0x19, 0xff, 0xff}},
        {0x0001'0000, 5, {0x1a, 0x00, 0x01, 0x00, 0x00}},
        {0x7fff'ffff, 5, {0x1a, 0x7f, 0xff, 0xff, 0xff}},
        {0x8000'0000, 5, {0x1a, 0x80, 0x00, 0x00, 0x00}},
        {0xffff'ffff, 5, {0x1a, 0xff, 0xff, 0xff, 0xff}},
        {
         0x0000'0001'0000'0000, 9,
         {0x1b, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00},
         },
        {
         0x7fff'ffff'ffff'ffff, 9,
         {0x1b, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
         },
        {
         0x8000'0000'0000'0000, 9,
         {0x1b, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
         },
        {
         0xffff'ffff'ffff'ffff, 9,
         {0x1b, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
         },
};

constexpr item_sample_ct<long long> negint_samples[] = {
  // Appendix A.Examples
        {-10LL, 1, {0b001'01001}},
        {-100LL, 2, {0x38, 0x63}},
        {-1000LL, 3, {0x39, 0x03, 0xe7}},
 // boundary test cases
        {-0x01LL, 1, {0b001'00000}},
        {-1 - 0x17LL, 1, {0b001'10111}},
        {-1 - 0x18LL, 2, {0x38, 0x18}},
        {-1 - 0x7fLL, 2, {0x38, 0x7f}},
        {-1 - 0x80LL, 2, {0x38, 0x80}},
        {-1 - 0xffLL, 2, {0x38, 0xff}},
        {-1 - 0x100LL, 3, {0x39, 0x01, 0x00}},
        {-1 - 0x7fffLL, 3, {0x39, 0x7f, 0xff}},
        {-1 - 0x8000LL, 3, {0x39, 0x80, 0x00}},
        {-1 - 0xffffLL, 3, {0x39, 0xff, 0xff}},
        {-1 - 0x1'0000LL, 5, {0x3a, 0x00, 0x01, 0x00, 0x00}},
        {-1 - 0x7fff'ffffLL, 5, {0x3a, 0x7f, 0xff, 0xff, 0xff}},
        {-1 - 0x8000'0000LL, 5, {0x3a, 0x80, 0x00, 0x00, 0x00}},
        {-1 - 0xffff'ffffLL, 5, {0x3a, 0xff, 0xff, 0xff, 0xff}},
        {
         -1 - 0x1'0000'0000LL,
         9, {0x3b, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00},
         },
        {
         -1 - 0x7fff'ffff'ffff'ffffLL,
         9, {0x3b, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
         },
};

constexpr item_sample_ct<unsigned> binary_samples[] = {
        { 0U,      1,                           {0x40}},
        { 4U,      5,               {0x44, 1, 2, 3, 4}},
        {23U, 23 + 1,   {0x57, 1, 2, 3, 4, 5, 6, 7, 8}},
        {24U, 24 + 2, {0x58, 24U, 1, 2, 3, 4, 5, 6, 7}},
};

// clang-format off
constexpr item_sample_ct<std::u8string_view> u8string_samples[] = {
        {
         u8""sv,  1,
         {0x60},
         },
        {
         u8"some"sv,  5,
         {0x64, u8's', u8'o', u8'm', u8'e'},
         },
        {
         u8"hello world"sv, 12,
         {0x6b, u8'h', u8'e', u8'l', u8'l', u8'o', u8' ', u8'w', u8'o'},
         },
        {
         u8".hello world.hello world"sv, 26,
         {0x78, 0x18, u8'.', u8'h', u8'e', u8'l', u8'l', u8'o', u8' '},
         }
};
// clang-format on

constexpr item_sample_ct<float> float_single_samples[] = {
        {                 100000.0F, 5, {0xfa, 0x47, 0xc3, 0x50, 0x00}},
        {   3.4028234663852886e+38F, 5, {0xfa, 0x7f, 0x7f, 0xff, 0xff}},
        { limits<float>::infinity(), 5, {0xfa, 0x7f, 0x80, 0x00, 0x00}},
        {limits<float>::quiet_NaN(), 5, {0xfa, 0x7f, 0xc0, 0x00, 0x00}},
        {-limits<float>::infinity(), 5, {0xfa, 0xff, 0x80, 0x00, 0x00}},
};

constexpr item_sample_ct<double> float_double_samples[] = {
        {                        1.1,9, {0xfb, 0x3f, 0xf1, 0x99, 0x99, 0x99, 0x99, 0x99, 0x9a}                                     },
        {                    1.e+300, 9, {0xfb, 0x7e, 0x37, 0xe4, 0x3c, 0x88, 0x00, 0x75, 0x9c}},
        { limits<double>::infinity(),
         9, {0xfb, 0x7f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}                             },
        {limits<double>::quiet_NaN(),
         9, {0xfb, 0x7f, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}                             },
        {-limits<double>::infinity(),
         9, {0xfb, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}                             },
};

} // namespace dp_tests
