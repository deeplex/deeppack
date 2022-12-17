
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <dplx/cncr/tag_invoke.hpp>
#include <dplx/cncr/type_utils.hpp>

#include <dplx/dp/concepts.hpp>
#include <dplx/dp/fwd.hpp>
#include <dplx/dp/items/emit_context.hpp>
#include <dplx/dp/stream.hpp>
#include <dplx/dp/streams/void_stream.hpp>

namespace dplx::dp::detail
{

// clang-format off
template <typename T>
concept has_codec_size_of = requires(T const &t, emit_context const &ctx)
{
    typename codec<T>;
    { codec<T>::size_of(ctx, t) } noexcept
        -> std::same_as<std::uint64_t>;
};
// clang-format on

} // namespace dplx::dp::detail

namespace dplx::dp
{

inline constexpr struct encoded_size_of_fn
{
    // TODO remove
    template <typename T>
        // clang-format off
        requires (cncr::tag_invocable<encoded_size_of_fn, T &&>
                    && !detail::has_codec_size_of<std::remove_cvref_t<T>>)
    // clang-format on
    constexpr auto operator()(T &&value) const
            noexcept(cncr::nothrow_tag_invocable<encoded_size_of_fn, T &&>)
                    -> cncr::tag_invoke_result_t<encoded_size_of_fn, T &&>
    {
        return cncr::tag_invoke(*this, static_cast<T &&>(value));
    }

    template <typename T>
        requires detail::has_codec_size_of<cncr::remove_cref_t<T>>
    constexpr auto operator()(T &&value) const noexcept
    {
        using unqualified_type = cncr::remove_cref_t<T>;
        void_stream dummyStream{};
        emit_context const ctx{dummyStream};
        return codec<unqualified_type>::size_of(
                ctx, static_cast<unqualified_type const &>(value));
    }
    template <typename T>
        requires detail::has_codec_size_of<cncr::remove_cref_t<T>>
    constexpr auto operator()(emit_context const &ctx, T &&value) const noexcept
    {
        using unqualified_type = cncr::remove_cref_t<T>;
        return codec<unqualified_type>::size_of(
                ctx, static_cast<unqualified_type const &>(value));
    }
} encoded_size_of{};

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

    template <typename T>
        requires ng::encodable<cncr::remove_cref_t<T>>
    inline auto operator()(output_buffer &outStream, T &&value) const noexcept
            -> result<void>
    {
        emit_context const ctx{outStream};
        return codec<cncr::remove_cref_t<T>>::encode(
                ctx, static_cast<cncr::remove_cref_t<T> const &>(value));
    }
    template <typename T>
        requires ng::encodable<cncr::remove_cref_t<T>>
    inline auto operator()(emit_context const &ctx, T &&value) const noexcept
            -> result<void>
    {
        return codec<cncr::remove_cref_t<T>>::encode(
                ctx, static_cast<cncr::remove_cref_t<T> const &>(value));
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

} // namespace dplx::dp
