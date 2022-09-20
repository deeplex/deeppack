
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <concepts>
#include <cstddef>
#include <ranges>
#include <span>
#include <type_traits>

#include <dplx/cncr/tag_invoke.hpp>

#include <dplx/dp/disappointment.hpp>

namespace dplx::dp
{

using bytes = std::span<std::byte const>;
using writable_bytes = std::span<std::byte>;

} // namespace dplx::dp

// output stream definitions & concepts
namespace dplx::dp
{

inline constexpr struct write_fn
{
    // direct encoding variant
    template <typename Stream>
        requires cncr::tag_invocable<write_fn, Stream &, std::size_t const>
    auto operator()(Stream &stream, std::size_t const size) const noexcept(
            cncr::nothrow_tag_invocable<write_fn, Stream &, std::size_t const>)
            -> cncr::tag_invoke_result_t<write_fn, Stream &, std::size_t const>
    {
        return cncr::tag_invoke(*this, stream, size);
    }

    // bulk transfer variant (for payload data like strings)
    template <typename Stream>
        requires cncr::tag_invocable<write_fn,
                                     Stream &,
                                     std::byte const *,
                                     std::size_t const>
    auto operator()(Stream &stream,
                    std::byte const *data,
                    std::size_t const numBytes) const
            noexcept(cncr::nothrow_tag_invocable<write_fn,
                                                 Stream &,
                                                 std::byte const *,
                                                 std::size_t const>)
                    -> cncr::tag_invoke_result_t<write_fn,
                                                 Stream &,
                                                 std::byte const *,
                                                 std::size_t const>
    {
        return cncr::tag_invoke(*this, stream, data, numBytes);
    }
} write{};

inline constexpr unsigned minimum_guaranteed_write_size = 40;

inline constexpr struct commit_fn
{
    template <typename Stream, typename WriteProxy>
        requires cncr::tag_invocable<commit_fn, Stream &, WriteProxy &>
    auto operator()(Stream &stream, WriteProxy &proxy) const noexcept(
            cncr::nothrow_tag_invocable<commit_fn, Stream &, WriteProxy &>)
            -> cncr::tag_invoke_result_t<commit_fn, Stream &, WriteProxy &>
    {
        return cncr::tag_invoke(*this, stream, proxy);
    }

    template <typename Stream, typename WriteProxy>
        requires cncr::tag_invocable<commit_fn,
                                     Stream &,
                                     WriteProxy &,
                                     std::size_t const>
    auto
    operator()(Stream &stream, WriteProxy &proxy, std::size_t const size) const
            noexcept(cncr::nothrow_tag_invocable<commit_fn,
                                                 Stream &,
                                                 WriteProxy &,
                                                 std::size_t const>)
                    -> cncr::tag_invoke_result_t<commit_fn,
                                                 Stream &,
                                                 WriteProxy &,
                                                 std::size_t const>
    {
        return cncr::tag_invoke(*this, stream, proxy, size);
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
using write_proxy_t = detail::result_value_t<decltype(write(
        std::declval<Stream &>(), std::declval<std::size_t>()))>;

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

// output stream trait definitions for `stream_traits` further below
namespace dplx::dp::detail
{

template <typename Stream>
struct output_stream_traits
{
    static constexpr bool output = true;
    static constexpr bool lazy_output = lazy_output_stream<Stream>;

    static constexpr bool nothrow_write_direct = cncr::
            nothrow_tag_invocable<write_fn, Stream &, std::size_t const>;
    static constexpr bool nothrow_write_indirect
            = cncr::nothrow_tag_invocable<write_fn,
                                          Stream &,
                                          std::byte const *,
                                          std::size_t const>;
    static constexpr bool nothrow_write
            = nothrow_write_direct && nothrow_write_indirect;

    static constexpr bool nothrow_commit
            = cncr::nothrow_tag_invocable<
                      commit_fn,
                      Stream &,
                      write_proxy_t<Stream> &,
                      std::size_t const> && (!lazy_output || cncr::nothrow_tag_invocable<commit_fn, Stream &, write_proxy_t<Stream> &>);

    static constexpr bool nothrow_output = nothrow_write && nothrow_commit;
};

} // namespace dplx::dp::detail

// input stream definitions & concepts
namespace dplx::dp
{

inline constexpr struct read_fn
{
    template <typename Stream>
        requires cncr::tag_invocable<read_fn, Stream &, std::size_t const>
    auto operator()(Stream &stream, std::size_t const size) const noexcept(
            cncr::nothrow_tag_invocable<read_fn, Stream &, std::size_t const>)
            -> cncr::tag_invoke_result_t<read_fn, Stream &, std::size_t const>
    {
        return cncr::tag_invoke(*this, stream, size);
    }

    template <typename Stream>
        requires cncr::
                tag_invocable<read_fn, Stream &, std::byte *, std::size_t const>
    auto
    operator()(Stream &stream, std::byte *data, std::size_t const size) const
            noexcept(cncr::nothrow_tag_invocable<read_fn,
                                                 Stream &,
                                                 std::byte *,
                                                 std::size_t const>)
                    -> cncr::tag_invoke_result_t<read_fn,
                                                 Stream &,
                                                 std::byte *,
                                                 std::size_t const>
    {
        return cncr::tag_invoke(*this, stream, data, size);
    }
} read{};

inline constexpr unsigned minimum_guaranteed_read_size = 40;

inline constexpr struct consume_fn
{
    template <typename Stream, typename ReadProxy>
        requires cncr::tag_invocable<consume_fn,
                                     Stream &,
                                     ReadProxy &,
                                     std::size_t const>
    auto operator()(Stream &stream,
                    ReadProxy &proxy,
                    std::size_t const actualSize) const
            noexcept(cncr::nothrow_tag_invocable<consume_fn,
                                                 Stream &,
                                                 ReadProxy &,
                                                 std::size_t const>)
                    -> cncr::tag_invoke_result_t<consume_fn,
                                                 Stream &,
                                                 ReadProxy &,
                                                 std::size_t const>
    {
        return cncr::tag_invoke(*this, stream, proxy, actualSize);
    }

    template <typename Stream, typename ReadProxy>
        requires cncr::tag_invocable<consume_fn, Stream &, ReadProxy &>
    auto operator()(Stream &stream, ReadProxy &proxy) const noexcept(
            cncr::nothrow_tag_invocable<consume_fn, Stream &, ReadProxy &>)
            -> cncr::tag_invoke_result_t<consume_fn, Stream &, ReadProxy &>
    {
        return cncr::tag_invoke(*this, stream, proxy);
    }
} consume{};

inline constexpr struct skip_bytes_fn
{
    template <typename Stream>
        requires cncr::
                tag_invocable<skip_bytes_fn, Stream &, std::uint64_t const>
    auto operator()(Stream &stream, std::uint64_t const numBytes) const
            noexcept(cncr::nothrow_tag_invocable<skip_bytes_fn,
                                                 Stream &,
                                                 std::uint64_t const>

                     ) -> cncr::tag_invoke_result_t<skip_bytes_fn,
                                                    Stream &,
                                                    std::uint64_t const>
    {
        return cncr::tag_invoke(*this, stream, numBytes);
    }
} skip_bytes{};

inline constexpr struct available_input_size_fn
{
    template <typename Stream>
        requires cncr::tag_invocable<available_input_size_fn, Stream &>
    auto operator()(Stream &stream) const noexcept(
            cncr::nothrow_tag_invocable<available_input_size_fn, Stream &>)
            -> cncr::tag_invoke_result_t<available_input_size_fn, Stream &>
    {
        return cncr::tag_invoke(*this, stream);
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
        { skip_bytes(stream, numBytes) } -> detail::tryable;
        { available_input_size(stream) }
                -> detail::tryable_result<std::size_t>;
    };
// clang-format on

template <input_stream Stream>
using read_proxy_t = detail::result_value_t<decltype(read(
        std::declval<Stream &>(), std::declval<std::size_t>()))>;

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

namespace dplx::dp::detail
{

template <typename Stream>
struct input_stream_traits
{
    static constexpr bool input = true;
    static constexpr bool lazy_input = lazy_input_stream<Stream>;

    static constexpr bool nothrow_read_direct
            = cncr::nothrow_tag_invocable<read_fn, Stream &, std::size_t const>;
    static constexpr bool nothrow_read_indirect
            = cncr::nothrow_tag_invocable<read_fn,
                                          Stream &,
                                          std::byte *,
                                          std::size_t const>;
    static constexpr bool nothrow_read
            = nothrow_read_direct && nothrow_read_indirect;

    static constexpr bool nothrow_consume
            = cncr::nothrow_tag_invocable<
                      consume_fn,
                      Stream &,
                      read_proxy_t<Stream> &,
                      std::size_t const> && (!lazy_input || cncr::nothrow_tag_invocable<consume_fn, Stream &, read_proxy_t<Stream> &>);

    static constexpr bool nothrow_skip_bytes = cncr::
            nothrow_tag_invocable<skip_bytes_fn, Stream &, std::uint64_t const>;

    static constexpr bool nothrow_available_input_bytes
            = cncr::nothrow_tag_invocable<available_input_size_fn, Stream &>;

    static constexpr bool nothrow_input = nothrow_read && nothrow_consume
                                       && nothrow_skip_bytes
                                       && nothrow_available_input_bytes;
};

} // namespace dplx::dp::detail

namespace dplx::dp
{

template <typename T>
struct stream_traits
{
};

template <output_stream Stream>
struct stream_traits<Stream> : detail::output_stream_traits<Stream>
{
    static constexpr bool input = false;
};

template <input_stream Stream>
struct stream_traits<Stream> : detail::input_stream_traits<Stream>
{
    static constexpr bool output = false;
};

template <typename InOutStream>
    requires output_stream<InOutStream> && input_stream<InOutStream>
struct stream_traits<InOutStream>
    : detail::output_stream_traits<InOutStream>
    , detail::input_stream_traits<InOutStream>
{
};

} // namespace dplx::dp
