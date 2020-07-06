
// Copyright Henrik Steffen Gaßmann 2020.
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <type_traits>

#include <dplx/dp/detail/type_utils.hpp>
#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/concepts.hpp>
#include <dplx/dp/encoder/arg_list.hpp>
#include <dplx/dp/fwd.hpp>
#include <dplx/dp/stream.hpp>

namespace dplx::dp
{

// the encode APIs are not meant to participate in ADL and are therefore
// niebloids
inline constexpr struct encode_fn final
{
    template <output_stream Stream, typename T>
    auto operator()(Stream &outStream, T &&value) const -> result<void>
    {
        return basic_encoder<Stream, std::remove_cvref_t<T>>()(
            outStream, static_cast<T &&>(value));
    }

    template <output_stream Stream, typename T = void>
    class bound_type
    {
        Stream *mOutStream;

    public:
        explicit bound_type(Stream &outStream) noexcept
            : mOutStream(&outStream)
        {
        }

        auto operator()(detail::select_proper_param_type<T> value) const
            -> result<void>
        {
            return basic_encoder<Stream, std::remove_cvref_t<T>>()(*mOutStream,
                                                                   value);
        }
    };
    template <output_stream Stream>
    class bound_type<Stream, void>
    {
        Stream *mOutStream;

    public:
        explicit bound_type(Stream &outStream) noexcept
            : mOutStream(&outStream)
        {
        }

        template <typename T>
        auto operator()(T &&value) const -> result<void>
        {
            return basic_encoder<Stream, std::remove_cvref_t<T>>()(*mOutStream,
                                                                   value);
        }
    };

    template <output_stream Stream>
    static auto bind(Stream &outStream) -> bound_type<Stream>
    {
        return bound_type<Stream>(outStream);
    }
    template <typename T, output_stream Stream>
    static auto bind(Stream &outStream) -> bound_type<Stream, T>
    {
        return bound_type<Stream, T>(outStream);
    }
} encode;

inline constexpr struct encode_array_fn final
{
    // encodes value arguments into a CBOR array data item.
    // clang-format off
    template <output_stream Stream, typename... Ts>
        requires (... && encodable<Stream, std::remove_cvref_t<Ts>>)
    auto operator()(Stream &outStream, Ts &&... values) const -> result<void>
    // clang-format on
    {
        return detail::arg_list_encoder<
            Stream,
            detail::mp_list<std::remove_cvref_t<Ts>...>>::
            encode(outStream, static_cast<Ts &&>(values)...);
    }

    template <output_stream Stream, typename... TArgs>
    class bound_type
        : detail::arg_list_encoder<Stream, std::remove_cvref_t<TArgs>...>
    {
        using impl_type =
            detail::arg_list_encoder<Stream, std::remove_cvref_t<TArgs>...>;

    public:
        using impl_type::impl_type;
        using impl_type::operator();
    };

    template <output_stream Stream>
    class bound_type<Stream>
    {
        Stream *mOutStream;

    public:
        explicit bound_type(Stream &outStream)
            : mOutStream(&outStream)
        {
        }

        template <typename... Ts>
        auto operator()(Ts &&... values) const -> result<void>
        {
            using impl = detail::arg_list_encoder<
                Stream,
                detail::mp_list<std::remove_cvref_t<Ts>...>>;
            return impl::encode(*mOutStream, static_cast<Ts &&>(values)...);
        }
    };

    template <typename... TArgs, output_stream Stream>
    static auto bind(Stream &outStream) -> bound_type<Stream, TArgs...>
    {
        return bound_type<Stream, std::remove_cvref_t<TArgs>...>(outStream);
    }
} encode_array;

inline constexpr struct encode_map_t final
{
    // clang-format off
    template <output_stream Stream, typename... Ps>
        requires (... && pair_like<std::remove_reference_t<Ps>>)
    auto operator()(Stream &outStream, Ps &&... ps) const -> result<void>
    // clang-format on
    {
        DPLX_TRY(type_encoder<Stream>::map(outStream, sizeof...(Ps)));

        result<void> rx = success();

        [[maybe_unused]] bool failed =
            (... ||
             (rx = this->encode_pair<Stream>(outStream, static_cast<Ps &&>(ps)))
                 .has_failure());

        return rx;
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
        auto operator()(Ps &&... ps) const -> result<void>
        {
            DPLX_TRY(type_encoder<Stream>::map(*mOutStream, sizeof...(Ps)));

            result<void> rx = success();

            [[maybe_unused]] bool failed =
                (... || (rx = encode_map_t::encode_pair<Stream>(
                             *mOutStream, static_cast<Ps &&>(ps)))
                            .has_failure());

            return rx;
        }
    };

    template <output_stream Stream>
    static auto bind(Stream &outStream) -> bound_type<Stream>
    {
        return bound_type<Stream>(outStream);
    }

private:
    template <typename Stream, typename P>
    inline static auto encode_pair(Stream &stream, P &&p) -> result<void>
    {
        using std::get;

        using pair_type = std::remove_cvref_t<P>;
        using key_type =
            std::remove_cvref_t<std::tuple_element_t<0, pair_type>>;
        using value_type =
            std::remove_cvref_t<std::tuple_element_t<1, pair_type>>;

        DPLX_TRY((basic_encoder<Stream, key_type>()(stream, get<0>(p))));
        DPLX_TRY((basic_encoder<Stream, value_type>()(stream, get<1>(p))));
        return success();
    }
} encode_map;

// does not output anything if called without value arguments
// encodes a single value argument directly without an enclosing array
// encodes multiple value arguments into a CBOR array data item.
inline constexpr struct encode_varargs_t final
{
    template <output_stream Stream, typename... Ts>
    auto operator()([[maybe_unused]] Stream &outStream,
                    [[maybe_unused]] Ts &&... values) const -> result<void>
    {
        if constexpr (sizeof...(Ts) == 0)
        {
            return success();
        }
        else if constexpr (sizeof...(Ts) == 1)
        {
            return basic_encoder<Stream, std::remove_cvref_t<Ts>...>()(
                outStream, static_cast<Ts &&>(values)...);
        }
        else if constexpr (sizeof...(Ts) > 1)
        {
            return detail::arg_list_encoder<
                Stream,
                detail::mp_list<std::remove_cvref_t<Ts>...>>::
                encode(outStream, static_cast<Ts &&>(values)...);
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
        auto operator()(Ts &&... vs) const -> result<void>
        {
            return encode_varargs_t{}(*mOutStream, static_cast<Ts &&>(vs)...);
        }
    };

    template <output_stream Stream>
    static auto bind(Stream &outStream) -> bound_type<Stream>
    {
        return bound_type<Stream>(outStream);
    }
} encode_varargs;

} // namespace dplx::dp
