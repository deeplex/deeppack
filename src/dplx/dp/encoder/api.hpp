
// Copyright Henrik Steffen Gaßmann 2020.
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <type_traits>

#include <dplx/cncr/tag_invoke.hpp>
#include <dplx/cncr/type_utils.hpp>

#include <dplx/dp/concepts.hpp>
#include <dplx/dp/detail/type_utils.hpp>
#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/encoder/arg_list.hpp>
#include <dplx/dp/fwd.hpp>
#include <dplx/dp/stream.hpp>

namespace dplx::dp
{

// the encode APIs are not meant to participate in ADL and are therefore
// niebloids
inline constexpr struct encode_fn final
{
    template <typename T, output_stream Stream>
        requires encodable<cncr::remove_cref_t<T>, Stream>
    inline auto operator()(Stream &outStream, T &&value) const -> result<void>
    {
        return basic_encoder<cncr::remove_cref_t<T>, Stream>()(
                outStream, static_cast<T &&>(value));
    }

    template <typename T, output_stream Stream>
    class bound_type
    {
        Stream *mOutStream;

    public:
        inline explicit bound_type(Stream &outStream) noexcept
            : mOutStream(&outStream)
        {
        }

        inline auto operator()(detail::select_proper_param_type<T> value) const
                -> result<void>
        {
            return basic_encoder<cncr::remove_cref_t<T>, Stream>()(*mOutStream,
                                                                   value);
        }
    };
    template <output_stream Stream>
    class bound_type<void, Stream>
    {
        Stream *mOutStream;

    public:
        inline explicit bound_type(Stream &outStream) noexcept
            : mOutStream(&outStream)
        {
        }

        template <typename T>
            requires encodable<cncr::remove_cref_t<T>, Stream>
        inline auto operator()(T &&value) const -> result<void>
        {
            return basic_encoder<cncr::remove_cref_t<T>, Stream>()(*mOutStream,
                                                                   value);
        }
    };

    template <output_stream Stream>
    static inline auto bind(Stream &outStream) -> bound_type<void, Stream>
    {
        return bound_type<void, Stream>(outStream);
    }
    template <typename T, output_stream Stream>
        requires encodable<cncr::remove_cref_t<T>, Stream>
    static inline auto bind(Stream &outStream) -> bound_type<T, Stream>
    {
        return bound_type<T, Stream>(outStream);
    }
} encode{};

inline constexpr struct encode_array_fn final
{
    // encodes value arguments into a CBOR array data item.
    // clang-format off
    template <typename... Ts, output_stream Stream>
        requires (... && encodable<cncr::remove_cref_t<Ts>, Stream>)
    inline auto operator()(Stream &outStream, Ts &&... values) const
        -> result<void>
    // clang-format on
    {
        return detail::arg_list_encoder<
                cncr::mp_list<cncr::remove_cref_t<Ts>...>,
                Stream>::encode(outStream, static_cast<Ts &&>(values)...);
    }

    template <output_stream Stream, typename... TArgs>
        requires(... &&encodable<cncr::remove_cref_t<TArgs>, Stream>)
    class bound_type
        : detail::arg_list_encoder<cncr::mp_list<cncr::remove_cref_t<TArgs>...>,
                                   Stream>
    {
        using impl_type = detail::arg_list_encoder<
                cncr::mp_list<cncr::remove_cref_t<TArgs>...>,
                Stream>;

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
            requires(... &&encodable<cncr::remove_cref_t<Ts>, Stream>)
        auto operator()(Ts &&...values) const -> result<void>
        {
            using impl = detail::arg_list_encoder<
                    cncr::mp_list<cncr::remove_cref_t<Ts>...>, Stream>;
            return impl::encode(*mOutStream, static_cast<Ts &&>(values)...);
        }
    };

    template <typename... TArgs, output_stream Stream>
    static auto bind(Stream &outStream) -> bound_type<Stream, TArgs...>
    {
        return bound_type<Stream, cncr::remove_cref_t<TArgs>...>(outStream);
    }
} encode_array{};

inline constexpr struct encode_map_t final
{
    // clang-format off
    template <typename... Ps, output_stream Stream>
    requires (... && (
        pair_like<cncr::remove_cref_t<Ps>>
        && encodable<cncr::remove_cref_t<
            typename std::tuple_element<0, cncr::remove_cref_t<Ps>>::type>,
            Stream>
        && encodable<cncr::remove_cref_t<
            typename std::tuple_element<1, cncr::remove_cref_t<Ps>>::type>,
            Stream>
        ))
    inline auto operator()(Stream &outStream, Ps &&... ps) const -> result<void>
    // clang-format on
    {
        DPLX_TRY(item_emitter<Stream>::map(outStream, sizeof...(Ps)));

        result<void> rx = success();

        [[maybe_unused]] bool failed
                = (...
                   || (rx = this->encode_pair<Ps, Stream>(
                               outStream, static_cast<Ps &&>(ps)))
                              .has_failure());

        return rx;
    }

    template <output_stream Stream>
    class bound_type
    {
        Stream *mOutStream;

    public:
        inline explicit bound_type(Stream &outStream)
            : mOutStream(&outStream)
        {
        }

        // clang-format off
        template <typename... Ps>
        requires (... && (
            pair_like<cncr::remove_cref_t<Ps>>
            && encodable<cncr::remove_cref_t<
                typename std::tuple_element<0, cncr::remove_cref_t<Ps>>::type>,
                Stream>
            && encodable<cncr::remove_cref_t<
                typename std::tuple_element<1, cncr::remove_cref_t<Ps>>::type>,
                Stream>
            ))
        inline auto operator()(Ps &&... ps) const -> result<void>
        // clang-format on
        {
            DPLX_TRY(item_emitter<Stream>::map(*mOutStream, sizeof...(Ps)));

            result<void> rx = success();

            [[maybe_unused]] bool failed
                    = (...
                       || (rx = encode_map_t::encode_pair<Ps, Stream>(
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
    template <typename P, output_stream Stream>
    inline static auto encode_pair(Stream &stream, P &&p) -> result<void>
    {
        using std::get;

        using pair_type = cncr::remove_cref_t<P>;
        using key_type
                = cncr::remove_cref_t<std::tuple_element_t<0, pair_type>>;
        using value_type
                = cncr::remove_cref_t<std::tuple_element_t<1, pair_type>>;

        DPLX_TRY((basic_encoder<key_type, Stream>()(stream, get<0>(p))));
        DPLX_TRY((basic_encoder<value_type, Stream>()(stream, get<1>(p))));
        return success();
    }
} encode_map{};

inline constexpr struct encode_varargs_t final
{
    // does not output anything if called without value arguments
    // encodes a single value argument directly without an enclosing array
    // encodes multiple value arguments into a CBOR array data item.
    template <typename... Ts, output_stream Stream>
        requires(... &&encodable<cncr::remove_cref_t<Ts>, Stream>)
    inline auto operator()([[maybe_unused]] Stream &outStream,
                           [[maybe_unused]] Ts &&...values) const
            -> result<void>
    {
        if constexpr (sizeof...(Ts) == 0)
        {
            return success();
        }
        else if constexpr (sizeof...(Ts) == 1)
        {
            return basic_encoder<cncr::remove_cref_t<Ts>..., Stream>()(
                    outStream, static_cast<Ts &&>(values)...);
        }
        else if constexpr (sizeof...(Ts) > 1)
        {
            return detail::arg_list_encoder<
                    cncr::mp_list<cncr::remove_cref_t<Ts>...>,
                    Stream>::encode(outStream, static_cast<Ts &&>(values)...);
        }
    }

    template <output_stream Stream>
    class bound_type final
    {
        Stream *mOutStream;

    public:
        inline explicit bound_type(Stream &outStream)
            : mOutStream(&outStream)
        {
        }

        template <typename... Ts>
            requires(... &&encodable<cncr::remove_cref_t<Ts>, Stream>)
        inline auto operator()(Ts &&...vs) const -> result<void>
        {
            return encode_varargs_t{}(*mOutStream, static_cast<Ts &&>(vs)...);
        }
    };

    template <output_stream Stream>
    static inline auto bind(Stream &outStream) -> bound_type<Stream>
    {
        return bound_type<Stream>(outStream);
    }
} encode_varargs{};

inline constexpr struct encoded_size_of_fn
{
    template <typename T>
        requires cncr::tag_invocable<encoded_size_of_fn, T &&>
    constexpr auto operator()(T &&value) const
            noexcept(cncr::nothrow_tag_invocable<encoded_size_of_fn, T &&>)
                    -> cncr::tag_invoke_result_t<encoded_size_of_fn, T &&>
    {
        return cncr::tag_invoke(*this, static_cast<T &&>(value));
    }
} encoded_size_of{};

} // namespace dplx::dp
