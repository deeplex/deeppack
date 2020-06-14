
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

#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/tag_invoke.hpp>

namespace dplx::dp
{

// output stream definitions & concepts

inline constexpr struct write_fn
{
    // direct encoding variant
    template <typename Stream>
    requires tag_invocable<write_fn, Stream &, std::size_t const> auto
    operator()(Stream &stream, std::size_t const size) const
        noexcept(nothrow_tag_invocable<write_fn, Stream &, std::size_t const>)
            -> tag_invoke_result_t<write_fn, Stream &, std::size_t const>
    {
        return ::dplx::dp::cpo::tag_invoke(*this, stream, size);
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
        return ::dplx::dp::cpo::tag_invoke(*this, stream, bytes, numBytes);
    }
} write{};

inline constexpr unsigned int minimum_guaranteed_write_size = 40;

inline constexpr struct commit_fn
{
    template <typename Stream, typename WriteProxy>
    requires tag_invocable<commit_fn, Stream &, WriteProxy &> auto
    operator()(Stream &stream, WriteProxy &proxy) const
        noexcept(nothrow_tag_invocable<commit_fn, Stream &, WriteProxy &>)
            -> tag_invoke_result_t<commit_fn, Stream &, WriteProxy &>
    {
        return ::dplx::dp::cpo::tag_invoke(*this, stream, proxy);
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
        return ::dplx::dp::cpo::tag_invoke(*this, stream, proxy, size);
    }
} commit;

// clang-format off
template <typename Stream, typename T>
concept write_proxy
    = std::ranges::contiguous_range<T>
    && std::convertible_to<T, std::span<std::byte>>
    && requires(Stream &stream, T &proxy, std::size_t const size)
    {
        { commit(stream, proxy, size) } -> oc::concepts::basic_result;
    };
// clang-format on

// clang-format off
template <typename T, typename Stream>
concept write_result
    = oc::concepts::basic_result<T> && write_proxy<Stream, typename T::value_type>;
// clang-format on

// clang-format off
template <typename Stream>
concept output_stream
    = requires(Stream &stream, std::byte const *bytes, std::size_t const size)
    {
        {::dplx::dp::write(stream, size)} -> write_result<Stream>;
        {::dplx::dp::write(stream, bytes, size)} -> oc::concepts::basic_result;
    };
// clang-format on

// clang-format off
template <typename Stream, typename Proxy>
concept lazy_write_proxy
    = write_proxy<Stream, Proxy>
    && requires (Stream &stream, Proxy &proxy)
    {
        { ::dplx::dp::commit(stream, proxy) } -> oc::concepts::basic_result;
    };
// clang-format on

// clang-format off
template <typename T, typename Stream>
concept lazy_write_result
    = write_result<T, Stream>
    && lazy_write_proxy<Stream, typename T::value_type>;
// clang-format on

// clang-format off
template <typename Stream>
concept lazy_output_stream
    = output_stream<Stream>
    && requires (Stream &stream, std::size_t size)
    {
        { ::dplx::dp::write(stream, size) } -> lazy_write_result<Stream>;
    };
// clang-format on

// input stream definitions & concepts

inline constexpr struct read_fn
{
    template <typename Stream>
    requires tag_invocable<read_fn, Stream &, std::size_t const> auto
    operator()(Stream &stream, std::size_t const size) const
        noexcept(nothrow_tag_invocable<read_fn, Stream &, std::size_t const>)
            -> tag_invoke_result_t<read_fn, Stream &, std::size_t const>
    {
        return ::dplx::dp::cpo::tag_invoke(*this, stream, size);
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
        return ::dplx::dp::cpo::tag_invoke(*this, stream, data, size);
    }
} read;

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
        return ::dplx::dp::cpo::tag_invoke(*this, stream, proxy, actualSize);
    }

    template <typename Stream, typename ReadProxy>
    requires tag_invocable<consume_fn, Stream &, ReadProxy &> auto
    operator()(Stream &stream, ReadProxy &proxy) const
        noexcept(nothrow_tag_invocable<consume_fn, Stream &, ReadProxy &>)
            -> tag_invoke_result_t<consume_fn, Stream &, ReadProxy &>
    {
        return ::dplx::dp::cpo::tag_invoke(*this, stream, proxy);
    }
} consume;

inline constexpr struct available_input_size_fn
{
    template <typename Stream>
    requires tag_invocable<available_input_size_fn, Stream &> auto
    operator()(Stream &stream) const
        noexcept(nothrow_tag_invocable<available_input_size_fn, Stream &>)
            -> tag_invoke_result_t<available_input_size_fn, Stream &>
    {
        return ::dplx::dp::cpo::tag_invoke(*this, stream);
    }
} available_input_size;

// clang-format off
template <typename Proxy, typename Stream>
concept read_proxy
    = std::ranges::contiguous_range<Proxy>
    && std::convertible_to<Proxy, std::span<std::byte const>>
    && requires (Stream &stream, Proxy &proxy, std::size_t const actualSize)
    {
        { ::dplx::dp::consume(stream, proxy, actualSize) } -> oc::concepts::basic_result;
    };
// clang-format on

// clang-format off
template <typename T, typename Stream>
concept read_result
    = oc::concepts::basic_result<T> && read_proxy<typename T::value_type, Stream>;
// clang-format on

// clang-format off
template <typename T>
concept available_input_size_result
    = oc::concepts::basic_result<T> && std::convertible_to<typename T::value_type, std::size_t>;
// clang-format on


// clang-format off
template <typename Stream>
concept input_stream
    = requires(Stream &stream, std::byte *buffer, std::size_t const size)
    {
        { ::dplx::dp::read(stream, size) } -> read_result<Stream>;
        { ::dplx::dp::read(stream, buffer, size) } -> oc::concepts::basic_result;
        { ::dplx::dp::available_input_size(stream) }
            -> available_input_size_result;
    };
// clang-format on

// clang-format off
template <typename Proxy, typename Stream>
concept lazy_read_proxy
    = read_proxy<Proxy, Stream>
    && requires (Stream &stream, Proxy &proxy)
    {
        { ::dplx::dp::consume(stream, proxy) } -> oc::concepts::basic_result;
    };
// clang-format on

// clang-format off
template <typename T, typename Stream>
concept lazy_read_result
    = read_result<T, Stream>
    && lazy_read_proxy<typename T::value_type, Stream>;
// clang-format on

// clang-format off
template <typename Stream>
concept lazy_input_stream
    = input_stream<Stream>
    && requires (Stream &stream, std::size_t size)
    {
        { ::dplx::dp::read(stream, size) } -> lazy_read_result<Stream>;
    };
// clang-format on

} // namespace dplx::dp
