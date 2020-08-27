
// Copyright Henrik Steffen Gaßmann 2020.
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
#include <dplx/dp/detail/type_utils.hpp>
#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/encoder/arg_list.hpp>
#include <dplx/dp/fwd.hpp>
#include <dplx/dp/item_emitter.hpp>
#include <dplx/dp/map_pair.hpp>

namespace dplx::dp
{

// volatile types are not supported.
template <typename T, output_stream Stream>
class basic_encoder<T volatile, Stream>;
template <typename T, output_stream Stream>
class basic_encoder<T const volatile, Stream>;

template <output_stream Stream>
class basic_encoder<null_type, Stream>
{
public:
    inline auto operator()(Stream &outStream, null_type) -> result<void>
    {
        return item_emitter<Stream>::null(outStream);
    }
};

template <typename... TArgs, output_stream Stream>
class basic_encoder<mp_varargs<TArgs...>, Stream>
{
    using impl =
        detail::arg_list_encoder<detail::mp_list<std::remove_cvref_t<TArgs>...>,
                                 Stream>;

public:
    inline auto
    operator()(Stream &outStream,
               detail::select_proper_param_type<TArgs>... args) const
        -> result<void>
    {
        return impl::encode(outStream, args...);
    }
};

template <output_stream Stream>
class basic_encoder<void, Stream>
{
public:
    template <typename T>
    inline auto operator()(Stream &outStream, T &&value) -> result<void>
    {
        return basic_encoder<std::remove_cvref_t<T>, Stream>()(
            outStream, static_cast<T &&>(value));
    }
};

template <output_stream Stream>
class basic_encoder<bool, Stream>
{
public:
    using value_type = bool;

    inline auto operator()(Stream &outStream, value_type value) -> result<void>
    {
        return item_emitter<Stream>::boolean(outStream, value);
    }
};

template <integer T, output_stream Stream>
class basic_encoder<T, Stream>
{
public:
    using value_type = T;

    auto operator()(Stream &outStream, value_type value) -> result<void>
    {
        return item_emitter<Stream>::integer(outStream, value);
    }
};

template <iec559_floating_point T, output_stream Stream>
class basic_encoder<T, Stream>
{
    static_assert(std::numeric_limits<T>::is_iec559);

public:
    using value_type = T;

    auto operator()(Stream &outStream, value_type value) -> result<void>
    {
        if constexpr (sizeof(value) == 4)
        {
            return item_emitter<Stream>::float_single(outStream, value);
        }
        else if constexpr (sizeof(value) == 8)
        {
            return item_emitter<Stream>::float_double(outStream, value);
        }
    }
};

// clang-format off
template <std::ranges::range T, output_stream Stream>
    requires encodable<std::ranges::range_value_t<T>, Stream>
class basic_encoder<T, Stream>
// clang-format on
{
    using wrapped_value_type = std::ranges::range_value_t<T>;
    using wrapped_encoder = basic_encoder<wrapped_value_type, Stream>;

public:
    using value_type = T;

    auto operator()(Stream &outStream, value_type const &value) const
        -> result<void>
    {
        if constexpr (enable_indefinite_encoding<T>)
        {
            DPLX_TRY(item_emitter<Stream>::array_indefinite(outStream));

            for (auto &&part : value)
            {
                DPLX_TRY(wrapped_encoder()(outStream,
                                           static_cast<decltype(part)>(part)));
            }

            return item_emitter<Stream>::break_(outStream);
        }
        else if constexpr (std::ranges::sized_range<T>)
        {
            DPLX_TRY(item_emitter<Stream>::array(outStream,
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
            DPLX_TRY(item_emitter<Stream>::array(outStream, size));

            for (; begin != end; ++begin)
            {
                DPLX_TRY(wrapped_encoder()(outStream, *begin));
            }

            return success();
        }
    }
};

// clang-format off
template <typename T, std::size_t N, output_stream Stream>
    requires encodable<T, Stream>
class basic_encoder<T[N], Stream> : basic_encoder<std::span<T const>, Stream>
// clang-format on
{
    using wrapped_encoder = basic_encoder<std::span<T const>, Stream>;

public:
    using value_type = T[N];
    using basic_encoder<std::span<T const>, Stream>::basic_encoder;
    using wrapped_encoder::operator();
};

// clang-format off
template <typename T, output_stream Stream>
    requires detail::encodable_tuple_like<T, Stream>
class basic_encoder<T, Stream>
// clang-format on
{
    using impl = detail::arg_list_encoder<
        detail::mp_transform_t<std::remove_cvref_t,
                               detail::mp_rename_t<T, detail::mp_list>>,
        Stream>;

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
template <associative_range T, output_stream Stream>
    requires encodable<std::ranges::range_value_t<T>, Stream>
class basic_encoder<T, Stream>
// clang-format on
{
    using pair_like = std::ranges::range_value_t<T>;
    using key_encoder =
        basic_encoder<std::remove_cvref_t<std::tuple_element_t<0, pair_like>>,
                      Stream>;
    using value_encoder =
        basic_encoder<std::remove_cvref_t<std::tuple_element_t<1, pair_like>>,
                      Stream>;

public:
    using value_type = T;

    auto operator()(Stream &outStream, value_type const &value) -> result<void>
    {
        if constexpr (enable_indefinite_encoding<T>)
        {
            DPLX_TRY(item_emitter<Stream>::map_indefinite(outStream));

            for (auto &&[k, v] : value)
            {
                DPLX_TRY(key_encoder()(outStream, k));
                DPLX_TRY(value_encoder()(outStream, v));
            }

            return item_emitter<Stream>::break_(outStream);
        }
        else if constexpr (std::ranges::sized_range<T>)
        {
            DPLX_TRY(
                item_emitter<Stream>::map(outStream, std::ranges::size(value)));

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
            DPLX_TRY(item_emitter<Stream>::map(outStream, size));

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
