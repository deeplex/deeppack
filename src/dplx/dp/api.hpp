
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <dplx/cncr/tag_invoke.hpp>
#include <dplx/cncr/type_utils.hpp>

#include <dplx/dp/concepts.hpp>
#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/fwd.hpp>
#include <dplx/dp/items/emit_context.hpp>
#include <dplx/dp/items/parse_context.hpp>
#include <dplx/dp/stream.hpp>
#include <dplx/dp/streams/void_stream.hpp>

namespace dplx::dp::detail
{

// clang-format off
template <typename T>
concept has_codec_size_of = requires(T const &t, emit_context const &ctx) // TODO: remove
{
    { codec<T>::size_of(ctx, t) } noexcept
        -> std::same_as<std::uint64_t>;
};
// clang-format on

} // namespace dplx::dp::detail

namespace dplx::dp
{

inline constexpr struct encoded_size_of_fn
{
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
} encode{};

// the decode APIs are not meant to participate in ADL and are therefore
// niebloids
inline constexpr struct decode_fn final
{
    template <typename T>
        requires ng::decodable<T>
    inline auto operator()(input_buffer &inStream, T &outValue) const noexcept
            -> result<void>
    {
        parse_context ctx{inStream};
        return codec<T>::decode(ctx, outValue);
    }
    template <typename T>
        requires ng::decodable<T>
    inline auto operator()(parse_context &ctx, T &outValue) const noexcept
            -> result<void>
    {
        return codec<T>::decode(ctx, outValue);
    }

    template <typename T>
        requires(ng::decodable<T> &&std::is_default_constructible_v<T>)
    inline auto operator()(as_value_t<T>, input_buffer &inStream) const noexcept
            -> result<T>
    {
        parse_context ctx{inStream};
        result<T> rx(oc::success());
        if (auto parseRx = codec<T>::decode(ctx, rx.assume_value());
            parseRx.has_failure()) [[unlikely]]
        {
            rx = static_cast<decltype(parseRx) &&>(parseRx).as_failure();
        }
        return rx;
    }
    template <typename T>
        requires(ng::decodable<T> &&std::is_default_constructible_v<T>)
    inline auto operator()(as_value_t<T>, parse_context &ctx) const noexcept
            -> result<T>
    {
        result<T> rx(oc::success());
        if (auto parseRx = codec<T>::decode(ctx, rx.assume_value());
            parseRx.has_failure()) [[unlikely]]
        {
            rx = static_cast<decltype(parseRx) &&>(parseRx).as_failure();
        }
        return rx;
    }

    // legacy begin
    template <typename T, input_stream Stream>
        requires decodable<T, Stream>
    inline auto operator()(Stream &inStream, T &dest) const -> result<void>
    {
        DPLX_TRY((basic_decoder<T, Stream>()(inStream, dest)));
        return success();
    }

    template <typename T, input_stream Stream>
        requires(decodable<T, Stream> &&std::is_default_constructible_v<T>
                         &&std::is_move_constructible_v<T>)
    inline auto operator()(as_value_t<T>, Stream &inStream) const -> result<T>
    {
        auto value = T();
        DPLX_TRY((basic_decoder<T, Stream>()(inStream, value)));
        return dp::success(std::move(value));
    }
    // legacy end
} decode{};

} // namespace dplx::dp
