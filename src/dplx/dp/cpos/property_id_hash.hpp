
// Copyright Henrik Steffen Gaßmann 2023
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include <dplx/cncr/math_supplement.hpp>
#include <dplx/cncr/tag_invoke.hpp>

#include <dplx/dp/codecs/fixed_u8string.hpp>
#include <dplx/dp/detail/hash.hpp>

namespace dplx::dp
{

inline constexpr struct property_id_hash_fn
{
    template <typename T>
        requires cncr::tag_invocable<property_id_hash_fn, T const &>
    constexpr auto operator()(T const &value) const noexcept(
            cncr::nothrow_tag_invocable<property_id_hash_fn, T const &>)
            -> std::uint64_t
    {
        return cncr::tag_invoke(*this, value);
    }
    template <typename T>
        requires cncr::
                tag_invocable<property_id_hash_fn, T const &, std::uint64_t>
            constexpr auto operator()(T const &value, std::uint64_t seed) const
            noexcept(cncr::nothrow_tag_invocable<property_id_hash_fn,
                                                 T const &,
                                                 std::uint64_t>)
                    -> std::uint64_t
    {
        return cncr::tag_invoke(*this, value, seed);
    }

    template <cncr::unsigned_integer T>
    friend constexpr auto tag_invoke(property_id_hash_fn, T value) noexcept
            -> std::uint64_t
    {
        return static_cast<std::uint64_t>(value);
    }
    template <cncr::integer T>
    friend constexpr auto
    tag_invoke(property_id_hash_fn, T value, std::uint64_t const seed) noexcept
            -> std::uint64_t
    {
        return detail::xxhash3(value, seed);
    }

    friend constexpr auto tag_invoke(property_id_hash_fn,
                                     std::u8string_view str,
                                     std::uint64_t const seed = 0) noexcept
            -> std::uint64_t
    {
        return detail::fnvx_hash(str.data(), str.size(), seed);
    }

} property_id_hash;

} // namespace dplx::dp
