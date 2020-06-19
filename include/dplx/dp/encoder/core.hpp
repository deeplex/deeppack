
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

#include <concepts>
#include <ranges>
#include <span>
#include <type_traits>

#include <dplx/dp/concepts.hpp>
#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/fwd.hpp>
#include <dplx/dp/map_pair.hpp>
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

namespace detail
{
template <output_stream Stream, typename T>
class arg_list_encoder;

template <output_stream Stream, typename... TArgs>
class arg_list_encoder<Stream, mp_list<TArgs...>>
{
    Stream *mOutStream;

public:
    explicit inline arg_list_encoder(Stream &outStream) noexcept
        : mOutStream(&outStream)
    {
    }

    static inline auto encode(Stream &outStream,
                              select_proper_param_type<TArgs>... values)
        -> result<void>
    {
        DPLX_TRY(type_encoder<Stream>::array(outStream, sizeof...(TArgs)));

        result<void> rx = success();

        [[maybe_unused]] bool failed =
            (... || (rx = basic_encoder<Stream, TArgs>()(outStream, values))
                        .has_failure());

        return rx;
    }

    auto inline operator()(select_proper_param_type<TArgs>... values) const
        -> result<void>
    {
        return encode(*mOutStream, values...);
    }
};
} // namespace detail

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

template <output_stream Stream>
class basic_encoder<Stream, null_type>
{
public:
    auto operator()(Stream &outStream, null_type) -> result<void>
    {
        return type_encoder<Stream>::null(outStream);
    }
};

template <output_stream Stream, typename... TArgs>
class basic_encoder<Stream, mp_varargs<TArgs...>>
{
    using impl = detail::arg_list_encoder<
        Stream,
        detail::mp_list<std::remove_cvref_t<TArgs>...>>;

public:
    auto operator()(Stream &outStream,
                    detail::select_proper_param_type<TArgs>... args) const
        -> result<void>
    {
        return impl::encode(outStream, args...);
    }
};

template <output_stream Stream>
class basic_encoder<Stream, void>
{
public:
    template <typename T>
    auto operator()(Stream &outStream, T &&value) -> result<void>
    {
        return basic_encoder<Stream, std::remove_cvref_t<T>>()(
            outStream, static_cast<T &&>(value));
    }
};

template <output_stream Stream>
class basic_encoder<Stream, bool>
{
public:
    using value_type = bool;

    auto operator()(Stream &outStream, value_type value) -> result<void>
    {
        return type_encoder<Stream>::boolean(outStream, value);
    }
};

template <output_stream Stream, integer T>
class basic_encoder<Stream, T>
{
public:
    using value_type = T;

    auto operator()(Stream &outStream, value_type value) -> result<void>
    {
        return type_encoder<Stream>::integer(outStream, value);
    }
};

template <output_stream Stream, iec559_floating_point T>
class basic_encoder<Stream, T>
{
    static_assert(std::numeric_limits<T>::is_iec559);

public:
    using value_type = T;

    auto operator()(Stream &outStream, value_type value) -> result<void>
    {
        if constexpr (sizeof(value) == 4)
        {
            return type_encoder<Stream>::float_single(outStream, value);
        }
        else if constexpr (sizeof(value) == 8)
        {
            return type_encoder<Stream>::float_double(outStream, value);
        }
    }
};

// clang-format off
template <output_stream Stream, std::ranges::range T>
    requires encodable<Stream, std::ranges::range_value_t<T>>
class basic_encoder<Stream, T>
// clang-format on
{
    using wrapped_value_type = std::ranges::range_value_t<T>;
    using wrapped_encoder = basic_encoder<Stream, wrapped_value_type>;

public:
    using value_type = T;

    auto operator()(Stream &outStream, value_type const &value) const
        -> result<void>
    {
        if constexpr (enable_indefinite_encoding<T>)
        {
            DPLX_TRY(type_encoder<Stream>::array_indefinite(outStream));

            for (auto &&part : value)
            {
                DPLX_TRY(wrapped_encoder()(outStream,
                                           static_cast<decltype(part)>(part)));
            }

            return type_encoder<Stream>::break_(outStream);
        }
        else if constexpr (std::ranges::sized_range<T>)
        {
            DPLX_TRY(type_encoder<Stream>::array(outStream,
                                                 std::ranges::size(value)));

            for (auto &&part : value)
            {
                DPLX_TRY(wrapped_encoder()(outStream,
                                           static_cast<decltype(part)>(part)));
            }

            return success();
        }
        else
        {
            auto begin = std::ranges::begin(value);
            auto const end = std::ranges::end(value);
            auto const size =
                static_cast<std::size_t>(std::distance(begin, end));
            DPLX_TRY(type_encoder<Stream>::array(outStream, size));

            for (; begin != end; ++begin)
            {
                DPLX_TRY(wrapped_encoder()(outStream, *begin));
            }

            return success();
        }
    }
};

// clang-format off
template <output_stream Stream, typename T, std::size_t N>
    requires encodable<Stream, T>
class basic_encoder<Stream, T[N]> : basic_encoder<Stream, std::span<T const>>
// clang-format on
{
    using wrapped_encoder = basic_encoder<Stream, std::span<T const>>;

public:
    using value_type = T[N];
    using basic_encoder<Stream, std::span<T const>>::basic_encoder;
    using wrapped_encoder::operator();
};

// clang-format off
template <output_stream Stream, typename T>
    requires detail::encodeable_tuple_like<Stream, T>
class basic_encoder<Stream, T>
// clang-format on
{
    using impl = detail::arg_list_encoder<
        Stream,
        detail::mp_transform_t<std::remove_cvref_t,
                               detail::mp_rename_t<T, detail::mp_list>>>;

public:
    using value_type = T;

    auto operator()(Stream &outStream,
                    detail::select_proper_param_type<value_type> value) const
        -> result<void>
    {
        return detail::apply_simply(impl(outStream), value);
    }
};

// clang-format off
template <output_stream Stream, associative_range T>
    requires encodable<Stream, std::ranges::range_value_t<T>>
class basic_encoder<Stream, T>
// clang-format on
{
    using pair_like = std::ranges::range_value_t<T>;
    using key_encoder =
        basic_encoder<Stream,
                      std::remove_cvref_t<std::tuple_element_t<0, pair_like>>>;
    using value_encoder =
        basic_encoder<Stream,
                      std::remove_cvref_t<std::tuple_element_t<1, pair_like>>>;

public:
    using value_type = T;

    auto operator()(Stream &outStream, value_type const &value) -> result<void>
    {
        if constexpr (enable_indefinite_encoding<T>)
        {
            DPLX_TRY(type_encoder<Stream>::map_indefinite(outStream));

            for (auto &&[k, v] : value)
            {
                DPLX_TRY(key_encoder()(outStream, k));
                DPLX_TRY(value_encoder()(outStream, v));
            }

            return type_encoder<Stream>::break_(outStream);
        }
        else if constexpr (std::ranges::sized_range<T>)
        {
            DPLX_TRY(
                type_encoder<Stream>::map(outStream, std::ranges::size(value)));

            for (auto &&[k, v] : value)
            {
                DPLX_TRY(key_encoder()(outStream, k));
                DPLX_TRY(value_encoder()(outStream, v));
            }

            return success();
        }
        else
        {
            auto begin = std::ranges::begin(value);
            auto const end = std::ranges::end(value);
            auto const size =
                static_cast<std::size_t>(std::distance(begin, end));
            DPLX_TRY(type_encoder<Stream>::map(outStream, size));

            for (; begin != end; ++begin)
            {
                auto &&[k, v] = *begin;
                DPLX_TRY(key_encoder()(outStream, k));
                DPLX_TRY(value_encoder()(outStream, v));
            }

            return success();
        }
    }
};

} // namespace dplx::dp
