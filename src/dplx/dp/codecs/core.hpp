
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

// defines the following encoder specializations
//  * null_type
//  * bool
//  * integer
//  * iec559 floating point

#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/fwd.hpp>

namespace dplx::dp
{

template <>
class codec<null_type>
{
public:
    static auto encode(emit_context const &ctx, null_type) noexcept
            -> result<void>;
};

template <>
class codec<signed char>
{
public:
    static auto encode(emit_context const &ctx, signed char value) noexcept
            -> result<void>;
};
template <>
class codec<unsigned char>
{
public:
    static auto encode(emit_context const &ctx, unsigned char value) noexcept
            -> result<void>;
};
template <>
class codec<short>
{
public:
    static auto encode(emit_context const &ctx, short value) noexcept
            -> result<void>;
};
template <>
class codec<unsigned short>
{
public:
    static auto encode(emit_context const &ctx, unsigned short value) noexcept
            -> result<void>;
};
template <>
class codec<int>
{
public:
    static auto encode(emit_context const &ctx, int value) noexcept
            -> result<void>;
};
template <>
class codec<unsigned>
{
public:
    static auto encode(emit_context const &ctx, unsigned value) noexcept
            -> result<void>;
};
template <>
class codec<long>
{
public:
    static auto encode(emit_context const &ctx, long value) noexcept
            -> result<void>;
};
template <>
class codec<unsigned long>
{
public:
    static auto encode(emit_context const &ctx, unsigned long value) noexcept
            -> result<void>;
};
template <>
class codec<long long>
{
public:
    static auto encode(emit_context const &ctx, long long value) noexcept
            -> result<void>;
};
template <>
class codec<unsigned long long>
{
public:
    static auto encode(emit_context const &ctx,
                       unsigned long long value) noexcept -> result<void>;
};

template <>
class codec<bool>
{
public:
    static auto encode(emit_context const &ctx, bool value) noexcept
            -> result<void>;
};

template <>
class codec<float>
{
public:
    static auto encode(emit_context const &ctx, float value) noexcept
            -> result<void>;
};

template <>
class codec<double>
{
public:
    static auto encode(emit_context const &ctx, double value) noexcept
            -> result<void>;
};

} // namespace dplx::dp