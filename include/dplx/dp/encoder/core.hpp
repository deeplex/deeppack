
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

// defines the following encoder specializations
//  * null_type
//  * mp_varargs<...>
//  * void -- operator() template argument deduction
//  * bool
//  * integer
//  * iec559 floating point

#pragma once

#include <dplx/dp/fwd.hpp>
#include <dplx/dp/type_encoder.hpp>

namespace dplx::dp
{

// volatile types are not supported.
template <output_stream Stream, typename T>
class basic_encoder<Stream, T volatile>;
template <output_stream Stream, typename T>
class basic_encoder<Stream, T const volatile>;

template <output_stream Stream>
class basic_encoder<Stream, null_type>
{
    Stream *mOutStream;

public:
    explicit basic_encoder(Stream &outStream)
        : mOutStream(&outStream)
    {
    }

    void operator()(null_type)
    {
        type_encoder<Stream>::null(*mOutStream);
    }
};

template <output_stream Stream, typename... TArgs>
class basic_encoder<Stream, mp_varargs<TArgs...>>
{
    Stream *mOutStream;

public:
    explicit basic_encoder(Stream &outStream)
        : mOutStream(&outStream)
    {
    }

    void operator()(detail::deduce_proper_param_type<TArgs>... args)
    {
        type_encoder<Stream>::array(*mOutStream, sizeof...(TArgs));
        (...,
         (void)basic_encoder<Stream, std::remove_reference_t<TArgs>>{
             *mOutStream}(args));
    }
};

template <output_stream Stream>
class basic_encoder<Stream, void>
{
    Stream *mOutStream;

public:
    explicit basic_encoder(Stream &outStream)
        : mOutStream(&outStream)
    {
    }

    template <typename T>
    void operator()(T &&value)
    {
        basic_encoder<Stream, std::remove_cvref_t<T>>{*mOutStream}(
            std::forward<T>(value));
    }
};

template <output_stream Stream>
class basic_encoder<Stream, bool>
{
    Stream *mOutStream;

public:
    using value_type = bool;

    explicit basic_encoder(Stream &outStream)
        : mOutStream(&outStream)
    {
    }

    void operator()(value_type value)
    {
        type_encoder<Stream>::boolean(*mOutStream, value);
    }
};

template <output_stream Stream, integer T>
class basic_encoder<Stream, T>
{
    Stream *mOutStream;

public:
    using value_type = T;

    explicit basic_encoder(Stream &outStream)
        : mOutStream(&outStream)
    {
    }

    void operator()(value_type value)
    {
        type_encoder<Stream>::integer(*mOutStream, value);
    }
};

template <output_stream Stream, iec559_floating_point T>
class basic_encoder<Stream, T>
{
    Stream *mOutStream;

public:
    using value_type = T;

    explicit basic_encoder(Stream &outStream)
        : mOutStream(&outStream)
    {
    }

    void operator()(value_type value)
    {
        if constexpr (sizeof(value) == 4)
        {
            type_encoder<Stream>::float_single(*mOutStream, value);
        }
        else if constexpr (sizeof(value) == 8)
        {
            type_encoder<Stream>::float_double(*mOutStream, value);
        }
    }
};

} // namespace dplx::dp
