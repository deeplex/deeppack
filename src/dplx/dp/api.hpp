
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <dplx/cncr/type_utils.hpp>

#include <dplx/dp/concepts.hpp>
#include <dplx/dp/cpos/stream.hpp>
#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/fwd.hpp>
#include <dplx/dp/items/emit_context.hpp>
#include <dplx/dp/items/parse_context.hpp>
#include <dplx/dp/streams/input_buffer.hpp>
#include <dplx/dp/streams/output_buffer.hpp>
#include <dplx/dp/streams/void_stream.hpp>

namespace dplx::dp
{

template <typename T>
concept output_stream
        = requires(T stream) { get_output_buffer(static_cast<T &&>(stream)); };

template <typename T>
concept input_stream
        = requires(T stream) { get_input_buffer(static_cast<T &&>(stream)); };

} // namespace dplx::dp

namespace dplx::dp
{

inline constexpr struct encoded_size_of_fn
{
    template <typename T>
        requires encodable<cncr::remove_cref_t<T>>
    constexpr auto operator()(T &&value) const noexcept
    {
        using unqualified_type = cncr::remove_cref_t<T>;
        void_stream dummyStream{};
        emit_context ctx{dummyStream};
        return codec<unqualified_type>::size_of(
                ctx, static_cast<unqualified_type const &>(value));
    }
    template <typename T>
        requires encodable<cncr::remove_cref_t<T>>
    constexpr auto operator()(emit_context &ctx, T &&value) const noexcept
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
    template <typename T, typename OutStream>
        requires encodable<cncr::remove_cref_t<T>>
              && output_stream<OutStream &&>
                 inline auto operator()(OutStream &&outStream,
                                        T &&value) const noexcept
                 -> result<void>
    {
        auto &&buffer = get_output_buffer(static_cast<OutStream &&>(outStream));
        emit_context ctx{static_cast<output_buffer &>(buffer)};
        DPLX_TRY(codec<cncr::remove_cref_t<T>>::encode(
                ctx, static_cast<cncr::remove_cref_t<T> const &>(value)));
        return static_cast<output_buffer &>(buffer).sync_output();
    }
    template <typename T>
        requires encodable<cncr::remove_cref_t<T>>
    inline auto operator()(output_buffer &outStream, T &&value) const noexcept
            -> result<void>
    {
        emit_context ctx{outStream};
        return codec<cncr::remove_cref_t<T>>::encode(
                ctx, static_cast<cncr::remove_cref_t<T> const &>(value));
    }
    template <typename T>
        requires encodable<cncr::remove_cref_t<T>>
    inline auto operator()(emit_context &ctx, T &&value) const noexcept
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
    template <typename T, typename InStream>
        requires decodable<T>
              && input_stream<InStream>
                 inline auto operator()(InStream &&inStream,
                                        T &outValue) const noexcept
                 -> result<void>
    {
        auto &&buffer = get_input_buffer(static_cast<InStream &&>(inStream));
        parse_context ctx{static_cast<input_buffer &>(buffer)};
        DPLX_TRY(codec<T>::decode(ctx, outValue));
        return static_cast<input_buffer &>(buffer).sync_input();
    }
    template <typename T>
        requires decodable<T>
    inline auto operator()(input_buffer &inStream, T &outValue) const noexcept
            -> result<void>
    {
        parse_context ctx{inStream};
        return codec<T>::decode(ctx, outValue);
    }
    template <typename T>
        requires decodable<T>
    inline auto operator()(parse_context &ctx, T &outValue) const noexcept
            -> result<void>
    {
        return codec<T>::decode(ctx, outValue);
    }

    template <typename T, typename InStream>
        requires value_decodable<T> && std::default_initializable<T>
              && std::movable<T>
              && input_stream<InStream>
                 inline auto operator()(as_value_t<T>,
                                        InStream &&inStream) const noexcept
                 -> result<T>
    {
        auto &&buffer = get_input_buffer(static_cast<InStream &&>(inStream));
        parse_context ctx{static_cast<input_buffer &>(buffer)};
        result<T> decodeRx = (*this)(as_value<T>, ctx);
        if (decodeRx.has_value())
        {
            DPLX_TRY(static_cast<input_buffer &>(buffer).sync_input());
        }
        return decodeRx;
    }
    template <typename T>
        requires value_decodable<T>
    inline auto operator()(as_value_t<T>, input_buffer &inStream) const noexcept
            -> result<T>
    {
        parse_context ctx{inStream};
        return (*this)(as_value<T>, ctx);
    }
    template <typename T>
        requires value_decodable<T>
    inline auto operator()(as_value_t<T>, parse_context &ctx) const noexcept
            -> result<T>
    {
        // clang-format off
        if constexpr (requires {
                          { codec<T>::decode(ctx) } noexcept
                              -> std::same_as<result<T>>;
                      })
        // clang-format on
        {
            return codec<T>::decode(ctx);
        }
        // clang-format off
        else if constexpr (requires {
                              { codec<T>::decode(ctx) } noexcept
                                  -> cncr::tryable_result<T>;
                           })
        // clang-format on
        {
            if (auto parseRx = codec<T>::decode(ctx);
                outcome::try_operation_has_value(parseRx)) [[likely]]
            {
                return outcome::try_operation_extract_value(
                        static_cast<decltype(parseRx) &&>(parseRx));
            }
            else [[unlikely]]
            {
                return outcome::try_operation_return_as(
                        static_cast<decltype(parseRx) &&>(parseRx));
            }
        }
        else
        {
            result<T> rx(outcome::success());
            if (auto parseRx = codec<T>::decode(ctx, rx.assume_value());
                parseRx.has_error()) [[unlikely]]
            {
                rx = static_cast<decltype(parseRx) &&>(parseRx).assume_error();
            }
            return rx;
        }
    }
} decode{};

} // namespace dplx::dp
