
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

#include <dplx/dp/concepts.hpp>
#include <dplx/dp/fwd.hpp>
#include <dplx/dp/type_encoder.hpp>

namespace dplx::dp
{

// volatile types are not supported.
template <output_stream Stream, typename T>
class basic_encoder<Stream, T volatile>;
template <output_stream Stream, typename T>
class basic_encoder<Stream, T const volatile>;

// the encode APIs are not meant to participate in ADL and are therefore
// niebloids
struct encode_t final
{
    template <output_stream Stream, typename T>
    void operator()(Stream &outStream, T &&value) const
    {
        basic_encoder<Stream, std::remove_cvref_t<T>>{outStream}(
            static_cast<T &&>(value));
    }

    template <output_stream Stream, typename T = void>
    using bound_type = basic_encoder<Stream, T>;

    template <output_stream Stream>
    static auto bind(Stream &outStream) -> bound_type<Stream>
    {
        return bound_type<Stream>{outStream};
    }
    template <typename T, output_stream Stream>
    static auto bind(Stream &outStream)
        -> bound_type<Stream, T> requires encodeable<Stream, T>
    {
        return bound_type<Stream, T>{outStream};
    }
};
inline constexpr encode_t encode{};

struct encode_array_t final
{
    // encodes value arguments into a CBOR array data item.
    template <output_stream Stream, typename... Ts>
    void operator()(Stream &outStream, Ts &&... values) const
        requires(... &&encodeable<Stream, std::remove_cvref_t<Ts>>)
    {
        basic_encoder<Stream, mp_varargs<std::remove_cvref_t<Ts>...>>{
            outStream}(static_cast<Ts &&>(values)...);
    }

    template <output_stream Stream>
    class bound_type
    {
        Stream *mOutStream;

    public:
        explicit bound_type(Stream &outStream)
            : mOutStream(&outStream)
        {
        }

        template <typename... Ts>
        void operator()(Ts &&... values) const
        {
            basic_encoder<Stream, mp_varargs<std::remove_cvref_t<Ts>...>>{
                *mOutStream}(static_cast<Ts &&>(values)...);
        }
    };

    template <output_stream Stream>
    static auto bind(Stream &outStream) -> bound_type<Stream>
    {
        return bound_type<Stream>(outStream);
    }
};
inline constexpr encode_array_t encode_array{};

struct encode_map_t final
{
    template <output_stream Stream, typename... Ps>
    void operator()(Stream &outStream, Ps &&... ps) const
        requires(... &&pair_like<std::remove_reference_t<Ps>>)
    {
        type_encoder<Stream>::map(outStream, sizeof...(Ps));

        (..., this->encode_pair<Stream>(outStream, static_cast<Ps &&>(ps)));
    }

    template <output_stream Stream>
    class bound_type
    {
        Stream *mOutStream;

    public:
        explicit bound_type(Stream &outStream)
            : mOutStream(&outStream)
        {
        }

        template <typename... Ps>
        void operator()(Ps &&... ps) const
        {
            type_encoder<Stream>::map(*mOutStream, sizeof...(Ps));

            (..., encode_map_t::encode_pair(*mOutStream, ps));
        }
    };

    template <output_stream Stream>
    static auto bind(Stream &outStream) -> bound_type<Stream>
    {
        return bound_type<Stream>(outStream);
    }

private:
    template <typename Stream, typename P>
    inline static void encode_pair(Stream &stream, P &&p)
    {
        using std::get;

        using pair_type = std::remove_cvref_t<P>;
        using key_type =
            std::remove_cvref_t<std::tuple_element_t<0, pair_type>>;
        using value_type =
            std::remove_cvref_t<std::tuple_element_t<1, pair_type>>;

        basic_encoder<Stream, key_type>{stream}(get<0>(p));
        basic_encoder<Stream, value_type>{stream}(get<1>(p));
    }
};
inline constexpr encode_map_t encode_map{};

struct encode_varargs_t final
{
    template <output_stream Stream, typename... Ts>
    void operator()([[maybe_unused]] Stream &outStream,
                    [[maybe_unused]] Ts &&... values) const
    {
        // nothing if sizeof...(Ts) == 0
        if constexpr (sizeof...(Ts) == 1)
        {
            basic_encoder<Stream, std::remove_cvref_t<Ts>...>{outStream}(
                static_cast<Ts &&>(values)...);
        }
        else if constexpr (sizeof...(Ts) > 1)
        {
            basic_encoder<Stream, mp_varargs<std::remove_cvref_t<Ts>...>>{
                outStream}(static_cast<Ts &&>(values)...);
        }
    }

    template <output_stream Stream>
    class bound_type final
    {
        Stream *mOutStream;

    public:
        explicit bound_type(Stream &outStream)
            : mOutStream(&outStream)
        {
        }

        template <typename... Ts>
        void operator()(Ts &&... vs) const
        {
            encode_varargs_t{}(*mOutStream, static_cast<Ts &&>(vs)...);
        }
    };

    template <output_stream Stream>
    static auto bind(Stream &outStream) -> bound_type<Stream>
    {
        return bound_type<Stream>(outStream);
    }
};
// does not output anything if called without value arguments
// encodes a single value argument directly without an enclosing array
// encodes multiple value arguments into a CBOR array data item.
inline constexpr encode_varargs_t encode_varargs{};

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

    void operator()(detail::deduce_proper_param_type<TArgs> const... args)
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
            static_cast<T &&>(value));
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
