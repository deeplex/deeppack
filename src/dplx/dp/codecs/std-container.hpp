
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <ranges>

#include <dplx/predef/compiler/clang.h>

#include <dplx/dp/api.hpp>
#include <dplx/dp/concepts.hpp>
#include <dplx/dp/customization.std.hpp>
#include <dplx/dp/detail/workaround.hpp>
#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/fwd.hpp>
#include <dplx/dp/items.hpp>

// clang doesn't defers substitution into member requires clause until clang-16
// see https://github.com/llvm/llvm-project/issues/44178
#define DPLX_DP_WORKAROUND_CLANG_44178                                         \
    DPLX_DP_WORKAROUND_TESTED_AT(DPLX_COMP_CLANG, 15, 0, 1)

// blobs
namespace dplx::dp
{

template <range R>
    requires(std::ranges::contiguous_range<R>
                     &&std::same_as<std::byte, std::ranges::range_value_t<R>>)
class codec<R>
{
public:
    static auto size_of(emit_context const &ctx, R const &value) noexcept
            -> std::uint64_t
    {
        return dp::item_size_of_binary(ctx, std::ranges::size(value));
    }
    static auto encode(emit_context const &ctx, R const &value) noexcept
            -> result<void>
    {
        return dp::emit_binary(ctx, std::ranges::data(value),
                               std::ranges::size(value));
    }
    static auto decode(parse_context &ctx, R &value) noexcept -> result<void>
        requires(container_traits<R>::resize
                         &&std::ranges::output_range<R, std::byte>)
    {
        result<std::size_t> parseRx = dp::parse_binary(ctx, value);
        if (parseRx.has_failure()) [[unlikely]]
        {
            (void)container_resize(value, std::size_t{});
            return static_cast<decltype(parseRx) &&>(parseRx).as_failure();
        }
        return dp::success();
    }
};

} // namespace dplx::dp

// sequence containers
namespace dplx::dp
{

#if DPLX_DP_WORKAROUND_CLANG_44178
namespace detail
{

template <typename C>
concept decodable_sequence_container
        = sequence_container<C> && ng::decodable<typename C::value_type>;

template <typename C>
concept decodable_associative_container
        = associative_container<C> && ng::decodable<typename C::value_type>;

} // namespace detail
#endif

template <range R>
class codec<R>
{
public:
    static auto size_of(emit_context const &ctx, R const &vs) noexcept
            -> std::uint64_t
            requires(std::ranges::input_range<R>
                             &&ng::encodable<std::ranges::range_value_t<R>>)
    {
        if constexpr (enable_indefinite_encoding<R>)
        {
            return dp::item_size_of_array_indefinite(ctx, vs,
                                                     dp::encoded_size_of);
        }
        else
        {
            return dp::item_size_of_array(ctx, vs, dp::encoded_size_of);
        }
    }
    static auto encode(emit_context const &ctx, R const &vs) noexcept
            -> result<void>
        requires(std::ranges::input_range<R>
                         &&ng::encodable<std::ranges::range_value_t<R>>)
    {
        if constexpr (enable_indefinite_encoding<R>)
        {
            return dp::emit_array_indefinite(ctx, vs, dp::encode);
        }
        else
        {
            return dp::emit_array(ctx, vs, dp::encode);
        }
    }
    static auto decode(parse_context &ctx, R &c) noexcept -> result<void>
        requires
#if DPLX_DP_WORKAROUND_CLANG_44178
                detail::decodable_sequence_container<R>
#else
                sequence_container<R> && ng::decodable<typename R::value_type>
#endif
        {
            c.clear();
            DPLX_TRY(dp::parse_array(ctx, c, decode_element));
            return dp::success();
        }
    static auto decode(parse_context &ctx, R &c) noexcept -> result<void>
        requires
#if DPLX_DP_WORKAROUND_CLANG_44178
                detail::decodable_associative_container<R>
#else
                associative_container<R> && ng::decodable<typename R::key_type>
#endif
        {
            c.clear();
            DPLX_TRY(dp::parse_array(ctx, c, decode_key));
            return dp::success();
        }

private:
    static auto decode_element(parse_context &ctx,
                               R &vs,
                               std::size_t const) noexcept -> result<void>
        requires
#if DPLX_DP_WORKAROUND_CLANG_44178
                detail::decodable_sequence_container<R>
#else
                sequence_container<R> && ng::decodable<typename R::value_type>
#endif
        {
            try
            {
                typename R::reference v = vs.emplace_back();
                return dp::decode(ctx, v);
            }
            catch (std::bad_alloc const &)
            {
                return errc::not_enough_memory;
            }
        }
    static auto decode_key(parse_context &ctx,
                           R &vs,
                           std::size_t const) noexcept -> result<void>
        requires
#if DPLX_DP_WORKAROUND_CLANG_44178
                detail::decodable_associative_container<R>
#else
                associative_container<R> && ng::decodable<typename R::key_type>
#endif
        {
            try
            {
                DPLX_TRY(auto &&key,
                         dp::decode(as_value<typename R::key_type>, ctx));

                std::pair<typename R::iterator, bool> it
                        = vs.emplace(static_cast<typename R::key_type &&>(key));
                if (!it.second)
                {
                    vs.clear();
                    return errc::duplicate_key;
                }
                return dp::success();
            }
            catch (std::bad_alloc const &)
            {
                return errc::not_enough_memory;
            }
        }
};

template <mapping_associative_container C>
class codec<C>
{
public:
    static auto size_of(emit_context const &ctx, C const &vs) noexcept
            -> std::uint64_t requires
            ng::encodable<typename C::key_type> && ng::encodable<
                    typename C::mapped_type>
    {
        if constexpr (enable_indefinite_encoding<C>)
        {
            return dp::item_size_of_map_indefinite(ctx, vs, size_of_pair);
        }
        else
        {
            return dp::item_size_of_map(ctx, vs, size_of_pair);
        }
    }
    static auto encode(emit_context const &ctx, C const &vs) noexcept
            -> result<void>
        requires ng::encodable<typename C::key_type> && ng::encodable<
                typename C::mapped_type>
        {
            if constexpr (enable_indefinite_encoding<C>)
            {
                return dp::emit_map_indefinite(ctx, vs, encode_pair);
            }
            else
            {
                return dp::emit_map(ctx, vs, encode_pair);
            }
        }
    static auto decode(parse_context &ctx, C &vs) noexcept -> result<void>
        requires ng::decodable<typename C::key_type> && ng::decodable<
                typename C::mapped_type>
        {
            vs.clear();
            DPLX_TRY(dp::parse_map(ctx, vs, decode_pair));
            return oc::success();
        }

private:
    static auto size_of_pair(emit_context const &ctx,
                             typename C::value_type const &pair) noexcept
            -> std::uint64_t requires
            ng::encodable<typename C::key_type> && ng::encodable<
                    typename C::mapped_type>
    {
        auto const &[key, mapped] = pair;
        return dp::encoded_size_of(ctx, key) + dp::encoded_size_of(ctx, mapped);
    }
    static auto encode_pair(emit_context const &ctx,
                            typename C::value_type const &pair) noexcept
            -> result<void>
        requires ng::encodable<typename C::key_type> && ng::encodable<
                typename C::mapped_type>
        {
            auto const &[key, mapped] = pair;
            DPLX_TRY(dp::encode(ctx, key));
            DPLX_TRY(dp::encode(ctx, mapped));
            return dp::success();
        }
    static auto decode_pair(parse_context &ctx,
                            C &vs,
                            std::size_t const) noexcept -> result<void>
        requires ng::decodable<typename C::key_type> && ng::decodable<
                typename C::mapped_type>
        {
            try
            {
                DPLX_TRY(auto &&key,
                         dp::decode(as_value<typename C::key_type>, ctx));
                DPLX_TRY(auto &&mapped,
                         dp::decode(as_value<typename C::mapped_type>, ctx));

                std::pair<typename C::iterator, bool> emplaceResult
                        = vs.emplace(static_cast<typename C::key_type &&>(key),
                                     static_cast<typename C::mapped_type &&>(
                                             mapped));
                if (!emplaceResult.second)
                {
                    vs.clear();
                    return errc::duplicate_key;
                }
                return dp::success();
            }
            catch (std::bad_alloc const &)
            {
                return errc::not_enough_memory;
            }
        }
};

} // namespace dplx::dp

namespace dplx::dp
{

namespace detail
{

template <typename T>
class fixed_size_binary_item_container_codec
{
public:
    static auto size_of(emit_context const &ctx,
                        std::span<std::byte const> value) noexcept
            -> std::uint64_t
    {
        return dp::item_size_of_binary(ctx, value.size());
    }
    static auto encode(emit_context const &ctx,
                       std::span<std::byte const> value) noexcept
            -> result<void>
    {
        return dp::emit_binary(ctx, value.data(), value.size());
    }
    static auto decode(parse_context &ctx, std::span<std::byte> value) noexcept
            -> result<void>
        requires(!std::is_const_v<T>)
    {
        result<std::size_t> parseRx = dp::parse_binary(ctx, value);
        if (parseRx.has_failure()) [[unlikely]]
        {
            std::memset(value.data(), 0, value.size());
            return static_cast<decltype(parseRx) &&>(parseRx).as_failure();
        }
        if (parseRx.assume_value() != value.size())
        {
            std::memset(value.data(), 0, value.size());
            return errc::tuple_size_mismatch;
        }
        return dp::success();
    }
};

template <typename T>
class fixed_size_container_codec
{
public:
    static auto size_of(emit_context const &ctx, std::span<T const> vs) noexcept
            -> std::uint64_t requires(ng::encodable<T>)
    {
        return dp::item_size_of_array(ctx, vs, dp::encoded_size_of);
    }
    static auto encode(emit_context const &ctx, std::span<T const> vs) noexcept
            -> result<void>
        requires(ng::encodable<T>)
    {
        return dp::emit_array(ctx, vs, dp::encode);
    }
    static auto decode(parse_context &ctx, std::span<T> value) noexcept
            -> result<void>
        requires(ng::decodable<T>)
    {
        result<std::size_t> parseRx
                = dp::parse_array(ctx, value, value.size(), decode_element);
        if (parseRx.has_failure()) [[unlikely]]
        {
            return static_cast<decltype(parseRx) &&>(parseRx).as_failure();
        }
        if (parseRx.assume_value() != value.size())
        {
            return errc::tuple_size_mismatch;
        }
        return dp::success();
    }

private:
    static auto decode_element(parse_context &ctx,
                               std::span<T> const value,
                               std::size_t const i) noexcept -> result<void>
        requires(ng::decodable<T>)
    {
        return dp::decode(ctx, value[i]);
    }
};

/*
template <typename T>
class fixed_size_associative_container_codec
{
    using key_type = std::tuple_element_t<0U, T>;
    using mapped_type = std::tuple_element_t<1U, T>;

public:
    static auto size_of(emit_context const &ctx, std::span<T const> vs) noexcept
            -> std::uint64_t requires(encodable_pair<T>)
    {
        return dp::item_size_of_map(ctx, vs, item_size_of_pair);
    }
    static auto encode(emit_context const &ctx, std::span<T const> vs) noexcept
            -> result<void>
        requires(encodable_pair<T>)
    {
        return dp::emit_map(ctx, vs, encode_pair);
    }
    static auto decode(parse_context &ctx, std::span<T> value) noexcept
            -> result<void>
        requires(!std::is_const_v<T> && decodable_pair<T>)
    {
        result<std::size_t> parseRx
                = dp::parse_map(ctx, value, value.size(), decode_pair);
        if (parseRx.has_failure()) [[unlikely]]
        {
            return static_cast<decltype(parseRx) &&>(parseRx).as_failure();
        }
        if (parseRx.assume_value() != value.size())
        {
            return errc::tuple_size_mismatch;
        }
        return dp::success();
    }

private:
    static auto item_size_of_pair(emit_context const &ctx,
                                  T const &value) noexcept -> result<void>
    {
        auto const &[key, mapped] = value;
        return encoded_size_of(ctx, key) + encoded_size_of(ctx, mapped);
    }
    static auto encode_pair(emit_context const &ctx, T const &value) noexcept
            -> result<void>
    {
        auto const &[key, mapped] = value;
        DPLX_TRY(encode(ctx, key));
        DPLX_TRY(encode(ctx, mapped));
        return oc::success();
    }
    static auto decode_pair(parse_context &ctx,
                            std::span<T> const value,
                            std::size_t const i) noexcept -> result<void>
        requires(!std::is_const_v<T> && decodable_pair<T>)
    {
        auto &[key, mapped] = value[i];
        DPLX_TRY(decode(ctx, key));
        DPLX_TRY(decode(ctx, mapped));
        return oc::success();
    }
};
*/

} // namespace detail

// we provide separate specializations for std::span and std::array to
// - check that sequences and blobs exactly match the container size
// - reduce binary bloat due to codec functions not being instantiated for
//      different `N`s but once
template <std::size_t N>
class codec<std::array<std::byte, N>>
    : public detail::fixed_size_binary_item_container_codec<std::byte>
{
};
template <std::size_t N>
class codec<std::array<std::byte const, N>>
    : public detail::fixed_size_binary_item_container_codec<std::byte const>
{
};
template <codable T, std::size_t N>
    requires range<std::array<T, N>>
class codec<std::array<T, N>> : public detail::fixed_size_container_codec<T>
{
};

template <std::size_t N>
class codec<std::span<std::byte, N>>
    : public detail::fixed_size_binary_item_container_codec<std::byte>
{
};
template <std::size_t N>
class codec<std::span<std::byte const, N>>
    : public detail::fixed_size_binary_item_container_codec<std::byte const>
{
};
template <codable T, std::size_t N>
    requires range<std::span<T, N>>
class codec<std::span<T, N>> : public detail::fixed_size_container_codec<T>
{
};

} // namespace dplx::dp
