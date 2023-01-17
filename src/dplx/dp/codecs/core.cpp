
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/codecs/core.hpp"

#include <dplx/dp/items/emit_core.hpp>
#include <dplx/dp/items/encoded_item_head_size.hpp>
#include <dplx/dp/items/item_size_of_core.hpp>
#include <dplx/dp/items/parse_core.hpp>

namespace dplx::dp
{

auto codec<null_type>::size_of(emit_context const &ctx, null_type) noexcept
        -> std::uint64_t
{
    return dp::item_size_of_null(ctx);
}
auto codec<null_type>::encode(emit_context const &ctx, null_type) noexcept
        -> result<void>
{
    return dp::emit_null(ctx);
}

auto codec<signed char>::size_of(emit_context const &ctx,
                                 signed char value) noexcept -> std::uint64_t
{
    return dp::item_size_of_integer(ctx, value);
}
auto codec<signed char>::encode(emit_context const &ctx,
                                signed char value) noexcept -> result<void>
{
    return dp::emit_integer(ctx, value);
}
auto codec<signed char>::decode(parse_context &ctx, signed char &value) noexcept
        -> result<void>
{
    return dp::parse_integer(ctx, value);
}
auto codec<unsigned char>::size_of(emit_context const &ctx,
                                   unsigned char value) noexcept
        -> std::uint64_t
{
    return dp::item_size_of_integer(ctx, value);
}
auto codec<unsigned char>::encode(emit_context const &ctx,
                                  unsigned char value) noexcept -> result<void>
{
    return dp::emit_integer(ctx, value);
}
auto codec<unsigned char>::decode(parse_context &ctx,
                                  unsigned char &value) noexcept -> result<void>
{
    return dp::parse_integer(ctx, value);
}
auto codec<short>::size_of(emit_context const &ctx, short value) noexcept
        -> std::uint64_t
{
    return dp::item_size_of_integer(ctx, value);
}
auto codec<short>::encode(emit_context const &ctx, short value) noexcept
        -> result<void>
{
    return dp::emit_integer(ctx, value);
}
auto codec<short>::decode(parse_context &ctx, short &value) noexcept
        -> result<void>
{
    return dp::parse_integer(ctx, value);
}
auto codec<unsigned short>::size_of(emit_context const &ctx,
                                    unsigned short value) noexcept
        -> std::uint64_t
{
    return dp::item_size_of_integer(ctx, value);
}
auto codec<unsigned short>::encode(emit_context const &ctx,
                                   unsigned short value) noexcept
        -> result<void>
{
    return dp::emit_integer(ctx, value);
}
auto codec<unsigned short>::decode(parse_context &ctx,
                                   unsigned short &value) noexcept
        -> result<void>
{
    return dp::parse_integer(ctx, value);
}
auto codec<int>::size_of(emit_context const &ctx, int value) noexcept
        -> std::uint64_t
{
    return dp::item_size_of_integer(ctx, value);
}
auto codec<int>::encode(emit_context const &ctx, int value) noexcept
        -> result<void>
{
    return dp::emit_integer(ctx, value);
}
auto codec<int>::decode(parse_context &ctx, int &value) noexcept -> result<void>
{
    return dp::parse_integer(ctx, value);
}
auto codec<unsigned>::size_of(emit_context const &ctx, unsigned value) noexcept
        -> std::uint64_t
{
    return dp::item_size_of_integer(ctx, value);
}
auto codec<unsigned>::encode(emit_context const &ctx, unsigned value) noexcept
        -> result<void>
{
    return dp::emit_integer(ctx, value);
}
auto codec<unsigned>::decode(parse_context &ctx, unsigned &value) noexcept
        -> result<void>
{
    return dp::parse_integer(ctx, value);
}
auto codec<long>::size_of(emit_context const &ctx, long value) noexcept
        -> std::uint64_t
{
    return dp::item_size_of_integer(ctx, value);
}
auto codec<long>::encode(emit_context const &ctx, long value) noexcept
        -> result<void>
{
    return dp::emit_integer(ctx, value);
}
auto codec<long>::decode(parse_context &ctx, long &value) noexcept
        -> result<void>
{
    return dp::parse_integer(ctx, value);
}
auto codec<unsigned long>::size_of(emit_context const &ctx,
                                   unsigned long value) noexcept
        -> std::uint64_t
{
    return dp::item_size_of_integer(ctx, value);
}
auto codec<unsigned long>::encode(emit_context const &ctx,
                                  unsigned long value) noexcept -> result<void>
{
    return dp::emit_integer(ctx, value);
}
auto codec<unsigned long>::decode(parse_context &ctx,
                                  unsigned long &value) noexcept -> result<void>
{
    return dp::parse_integer(ctx, value);
}
auto codec<long long>::size_of(emit_context const &ctx,
                               long long value) noexcept -> std::uint64_t
{
    return dp::item_size_of_integer(ctx, value);
}
auto codec<long long>::encode(emit_context const &ctx, long long value) noexcept
        -> result<void>
{
    return dp::emit_integer(ctx, value);
}
auto codec<long long>::decode(parse_context &ctx, long long &value) noexcept
        -> result<void>
{
    return dp::parse_integer(ctx, value);
}
auto codec<unsigned long long>::size_of(emit_context const &ctx,
                                        unsigned long long value) noexcept
        -> std::uint64_t
{
    return dp::item_size_of_integer(ctx, value);
}
auto codec<unsigned long long>::encode(emit_context const &ctx,
                                       unsigned long long value) noexcept
        -> result<void>
{
    return dp::emit_integer(ctx, value);
}
auto codec<unsigned long long>::decode(parse_context &ctx,
                                       unsigned long long &value) noexcept
        -> result<void>
{
    return dp::parse_integer(ctx, value);
}

auto codec<bool>::size_of(emit_context const &ctx, bool value) noexcept
        -> std::uint64_t
{
    return dp::item_size_of_boolean(ctx, value);
}
auto codec<bool>::encode(emit_context const &ctx, bool value) noexcept
        -> result<void>
{
    return dp::emit_boolean(ctx, value);
}
auto codec<bool>::decode(parse_context &ctx, bool &value) noexcept
        -> result<void>
{
    return dp::parse_boolean(ctx, value);
}

auto codec<float>::size_of(emit_context const &ctx, float value) noexcept
        -> std::uint64_t
{
    return dp::item_size_of_float_single(ctx, value);
}
auto codec<float>::encode(emit_context const &ctx, float value) noexcept
        -> result<void>
{
    return dp::emit_float_single(ctx, value);
}
auto codec<float>::decode(parse_context &ctx, float &value) noexcept
        -> result<void>
{
    return dp::parse_floating_point<float>(ctx, value);
}

auto codec<double>::size_of(emit_context const &ctx, double value) noexcept
        -> std::uint64_t
{
    return dp::item_size_of_float_double(ctx, value);
}
auto codec<double>::encode(emit_context const &ctx, double value) noexcept
        -> result<void>
{
    return dp::emit_float_double(ctx, value);
}
auto codec<double>::decode(parse_context &ctx, double &value) noexcept
        -> result<void>
{
    return dp::parse_floating_point<double>(ctx, value);
}

} // namespace dplx::dp
