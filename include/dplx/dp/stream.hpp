
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>

#include <concepts>
#include <ranges>
#include <span>
#include <type_traits>

#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/tag_invoke.hpp>

// output stream definitions & concepts
namespace dplx::dp
{

inline constexpr struct write_fn
{
    // direct encoding variant
    template <typename Stream>
    requires tag_invocable<write_fn, Stream &, std::size_t const> auto
    operator()(Stream &stream, std::size_t const size) const noexcept(
            nothrow_tag_invocable<write_fn, Stream &, std::size_t const>)
            -> tag_invoke_result_t<write_fn, Stream &, std::size_t const>
    {
        return cpo::tag_invoke(*this, stream, size);
    }

    // bulk transfer variant (for payload data like strings)
    template <typename Stream>
    requires tag_invocable<write_fn,
                           Stream &,
                           std::byte const *,
                           std::size_t const> auto
    operator()(Stream &stream,
               std::byte const *bytes,
               std::size_t const numBytes) const
            noexcept(nothrow_tag_invocable<write_fn,
                                           Stream &,
                                           std::byte const *,
                                           std::size_t const>)
                    -> tag_invoke_result_t<write_fn,
                                           Stream &,
                                           std::byte const *,
                                           std::size_t const>
    {
        return cpo::tag_invoke(*this, stream, bytes, numBytes);
    }
} write{};

inline constexpr unsigned minimum_guaranteed_write_size = 40;

inline constexpr struct commit_fn
{
    template <typename Stream, typename WriteProxy>
    requires tag_invocable<commit_fn, Stream &, WriteProxy &> auto
    operator()(Stream &stream, WriteProxy &proxy) const
            noexcept(nothrow_tag_invocable<commit_fn, Stream &, WriteProxy &>)
                    -> tag_invoke_result_t<commit_fn, Stream &, WriteProxy &>
    {
        return cpo::tag_invoke(*this, stream, proxy);
    }

    template <typename Stream, typename WriteProxy>
    requires tag_invocable<commit_fn,
                           Stream &,
                           WriteProxy &,
                           std::size_t const> auto
    operator()(Stream &stream, WriteProxy &proxy, std::size_t const size) const
            noexcept(nothrow_tag_invocable<commit_fn,
                                           Stream &,
                                           WriteProxy &,
                                           std::size_t const>)
                    -> tag_invoke_result_t<commit_fn,
                                           Stream &,
                                           WriteProxy &,
                                           std::size_t const>
    {
        return cpo::tag_invoke(*this, stream, proxy, size);
    }
} commit{};

namespace detail
{

// clang-format off
template <typename Stream, typename T>
concept write_proxy
    = std::ranges::contiguous_range<T>
    && std::ranges::sized_range<T>
    && std::convertible_to<
        std::remove_reference_t<std::ranges::range_reference_t<T>>(*)[],
        std::byte (*)[]>
    && std::is_nothrow_default_constructible_v<T>
    && std::is_nothrow_move_constructible_v<T>
    && std::is_nothrow_move_assignable_v<T>
    && std::is_trivially_destructible_v<T>
    && requires(Stream &stream, T &proxy, std::size_t const size)
    {
        { commit(stream, proxy, size) } -> detail::tryable;
    };
// clang-format on

// clang-format off
template <typename T, typename Stream>
concept write_result
    = detail::tryable<T> && write_proxy<Stream, detail::result_value_t<T>>;
// clang-format on

} // namespace detail

// clang-format off
template <typename Stream>
concept output_stream
    = requires(Stream &stream, std::byte const *bytes, std::size_t const size)
    {
        { write(stream, size) } -> detail::write_result<Stream>;
        { write(stream, bytes, size) } -> detail::tryable;
    };
// clang-format on

template <output_stream Stream>
using write_proxy_t = detail::result_value_t<decltype(
        write(std::declval<Stream &>(), std::declval<std::size_t>()))>;

namespace detail
{

// clang-format off
template <typename Stream, typename Proxy>
concept lazy_write_proxy
    = write_proxy<Stream, Proxy>
    && requires (Stream &stream, Proxy &proxy)
    {
        { commit(stream, proxy) } -> detail::tryable;
    };
// clang-format on

// clang-format off
template <typename T, typename Stream>
concept lazy_write_result
    = write_result<T, Stream>
    && lazy_write_proxy<Stream, typename T::value_type>;
// clang-format on

} // namespace detail

// clang-format off
template <typename Stream>
concept lazy_output_stream
    = output_stream<Stream>
    && requires (Stream &stream, std::size_t size)
    {
        { write(stream, size) } -> detail::lazy_write_result<Stream>;
    };
// clang-format on

} // namespace dplx::dp

// input stream definitions & concepts
namespace dplx::dp
{

inline constexpr struct read_fn
{
    template <typename Stream>
    requires tag_invocable<read_fn, Stream &, std::size_t const> auto
    operator()(Stream &stream, std::size_t const size) const noexcept(
            nothrow_tag_invocable<read_fn, Stream &, std::size_t const>)
            -> tag_invoke_result_t<read_fn, Stream &, std::size_t const>
    {
        return cpo::tag_invoke(*this, stream, size);
    }

    template <typename Stream>
    requires tag_invocable<read_fn,
                           Stream &,
                           std::byte *,
                           std::size_t const> auto
    operator()(Stream &stream, std::byte *data, std::size_t const size) const
            noexcept(nothrow_tag_invocable<read_fn,
                                           Stream &,
                                           std::byte *,
                                           std::size_t const>)
                    -> tag_invoke_result_t<read_fn,
                                           Stream &,
                                           std::byte *,
                                           std::size_t const>
    {
        return cpo::tag_invoke(*this, stream, data, size);
    }
} read{};

inline constexpr unsigned minimum_guaranteed_read_size = 40;

inline constexpr struct consume_fn
{
    template <typename Stream, typename ReadProxy>
    requires tag_invocable<consume_fn,
                           Stream &,
                           ReadProxy &,
                           std::size_t const> auto
    operator()(Stream &stream,
               ReadProxy &proxy,
               std::size_t const actualSize) const
            noexcept(nothrow_tag_invocable<consume_fn,
                                           Stream &,
                                           ReadProxy &,
                                           std::size_t const>)
                    -> tag_invoke_result_t<consume_fn,
                                           Stream &,
                                           ReadProxy &,
                                           std::size_t const>
    {
        return cpo::tag_invoke(*this, stream, proxy, actualSize);
    }

    template <typename Stream, typename ReadProxy>
    requires tag_invocable<consume_fn, Stream &, ReadProxy &> auto
    operator()(Stream &stream, ReadProxy &proxy) const
            noexcept(nothrow_tag_invocable<consume_fn, Stream &, ReadProxy &>)
                    -> tag_invoke_result_t<consume_fn, Stream &, ReadProxy &>
    {
        return cpo::tag_invoke(*this, stream, proxy);
    }
} consume{};

inline constexpr struct available_input_size_fn
{
    template <typename Stream>
    requires tag_invocable<available_input_size_fn, Stream &> auto
    operator()(Stream &stream) const
            noexcept(nothrow_tag_invocable<available_input_size_fn, Stream &>)
                    -> tag_invoke_result_t<available_input_size_fn, Stream &>
    {
        return cpo::tag_invoke(*this, stream);
    }
} available_input_size{};

namespace detail
{

// clang-format off
template <typename Proxy, typename Stream>
concept read_proxy
    = std::ranges::contiguous_range<Proxy>
    && std::ranges::sized_range<Proxy>
    && std::convertible_to<
        std::remove_reference_t<std::ranges::range_reference_t<Proxy>>(*)[],
        std::byte const (*)[]>
    && std::is_nothrow_default_constructible_v<Proxy>
    && std::is_nothrow_move_constructible_v<Proxy>
    && std::is_nothrow_move_assignable_v<Proxy>
    && std::is_trivially_destructible_v<Proxy>
    && requires (Stream &stream, Proxy &proxy, std::size_t const actualSize)
    {
        { consume(stream, proxy, actualSize) } -> detail::tryable;
    };
// clang-format on

// clang-format off
template <typename T, typename Stream>
concept read_result
    = detail::tryable<T> && read_proxy<detail::result_value_t<T>, Stream>;
// clang-format on

} // namespace detail

// clang-format off
template <typename Stream>
concept input_stream
    = requires(Stream &stream, std::byte *buffer, std::size_t const size, std::uint64_t const numBytes)
    {
        { read(stream, size) } -> detail::read_result<Stream>;
        { read(stream, buffer, size) } -> detail::tryable;
        { available_input_size(stream) }
                -> detail::tryable_result<std::size_t>;
    };
// clang-format on

template <input_stream Stream>
using read_proxy_t = detail::result_value_t<decltype(
        read(std::declval<Stream &>(), std::declval<std::size_t>()))>;

namespace detail
{

// clang-format off
template <typename Proxy, typename Stream>
concept lazy_read_proxy
    = read_proxy<Proxy, Stream>
    && requires (Stream &stream, Proxy &proxy)
    {
        { consume(stream, proxy) } -> detail::tryable;
    };
// clang-format on

// clang-format off
template <typename T, typename Stream>
concept lazy_read_result
    = read_result<T, Stream>
    && lazy_read_proxy<detail::result_value_t<T>, Stream>;
// clang-format on

} // namespace detail

// clang-format off
template <typename Stream>
concept lazy_input_stream
    = input_stream<Stream>
    && requires (Stream &stream, std::size_t size)
    {
        { read(stream, size) } -> detail::lazy_read_result<Stream>;
    };
// clang-format on

} // namespace dplx::dp
