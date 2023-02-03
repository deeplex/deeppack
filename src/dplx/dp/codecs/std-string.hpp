
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <string>
#include <string_view>

#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/fwd.hpp>

namespace dplx::dp
{

template <>
class codec<std::u8string_view>
{
public:
    static auto size_of(emit_context &ctx, std::u8string_view value) noexcept
            -> std::uint64_t;
    static auto encode(emit_context &ctx, std::u8string_view value) noexcept
            -> result<void>;
};

template <>
class codec<std::u8string>
{
public:
    static auto size_of(emit_context &ctx, std::u8string const &value) noexcept
            -> std::uint64_t;
    static auto encode(emit_context &ctx, std::u8string const &value) noexcept
            -> result<void>;
    static auto decode(parse_context &ctx, std::u8string &value) noexcept
            -> result<void>;
};

template <>
class codec<std::string_view>
{
public:
    static auto size_of(emit_context &ctx, std::string_view value) noexcept
            -> std::uint64_t;
    static auto encode(emit_context &ctx, std::string_view value) noexcept
            -> result<void>;
};

template <>
class codec<std::string>
{
public:
    static auto size_of(emit_context &ctx, std::string const &value) noexcept
            -> std::uint64_t;
    static auto encode(emit_context &ctx, std::string const &value) noexcept
            -> result<void>;
    static auto decode(parse_context &ctx, std::string &value) noexcept
            -> result<void>;
};

} // namespace dplx::dp
