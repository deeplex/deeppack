
// Copyright Henrik Steffen Gaßmann 2023
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>
#include <ranges>

#include <dplx/dp/cpos/container.hpp>
#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/fwd.hpp>
#include <dplx/dp/items/parse_context.hpp>
#include <dplx/dp/items/parse_core.hpp>
#include <dplx/dp/items/type_code.hpp>

namespace dplx::dp
{

namespace detail
{

template <typename Container>
concept blob_output_container = container_traits<Container>::resize
                             && std::ranges::contiguous_range<Container>;

template <typename Container>
inline auto parse_blob_indefinite(parse_context &ctx,
                                  Container &dest,
                                  std::size_t maxSize,
                                  type_code expectedType) noexcept
        -> result<std::size_t>;

template <bool AllowIndefiniteEncoding, typename Container>
inline auto parse_blob(parse_context &ctx,
                       Container &dest,
                       std::size_t const maxSize,
                       type_code const expectedType) noexcept
        -> result<std::size_t>
{
    result<item_head> headParseRx = dp::parse_item_head(ctx);
    if (headParseRx.has_error())
    {
        return static_cast<result<item_head> &&>(headParseRx).as_failure();
    }
    item_head const &head = headParseRx.assume_value();

    if (head.type != expectedType)
    {
        return errc::item_type_mismatch;
    }
    if (head.indefinite()) [[unlikely]]
    {
        if constexpr (AllowIndefiniteEncoding)
        {
            return detail::parse_blob_indefinite<Container>(ctx, dest, maxSize,
                                                            expectedType);
        }
        else
        {
            return errc::indefinite_item;
        }
    }
    if (ctx.in.input_size() < head.value)
    {
        // defend against amplification attacks exhausting main memory
        return errc::missing_data;
    }
    if (head.value > maxSize)
    {
        return errc::string_exceeds_size_limit;
    }

    auto const size = static_cast<std::size_t>(head.value);
    DPLX_TRY(container_resize_for_overwrite(dest, size));

    auto const memory = std::ranges::data(dest);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    DPLX_TRY(ctx.in.bulk_read(reinterpret_cast<std::byte *>(memory), size));
    return size;
}

template <typename Container>
inline auto parse_blob_indefinite(parse_context &ctx,
                                  Container &dest,
                                  std::size_t const maxSize,
                                  type_code const expectedType) noexcept
        -> result<std::size_t>
{
    std::size_t size = 0U;
    DPLX_TRY(container_resize(dest, size));

    for (;;)
    {
        DPLX_TRY(item_head const &chunkItem, dp::parse_item_head(ctx));

        if (chunkItem.is_special_break())
        {
            break;
        }
        if (chunkItem.type != expectedType)
        {
            return errc::invalid_indefinite_subitem;
        }

        if (ctx.in.input_size() < chunkItem.value)
        {
            // defend against amplification attacks exhausting main memory
            return errc::missing_data;
        }

        auto const newSize = size + chunkItem.value;
        if (newSize > maxSize || newSize < chunkItem.value)
        {
            return errc::string_exceeds_size_limit;
        }
        auto const chunkSize = static_cast<std::size_t>(chunkItem.value);

        DPLX_TRY(container_resize_for_overwrite(
                dest, static_cast<std::size_t>(newSize)));

        auto const memory = std::ranges::data(dest);

        // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
        // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        DPLX_TRY(ctx.in.bulk_read(reinterpret_cast<std::byte *>(memory) + size,
                                  chunkSize));
        // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)

        size = static_cast<std::size_t>(newSize);
    }

    return size;
}

} // namespace detail

// clang-format off
template <typename Container>
concept binary_item_container
    = detail::blob_output_container<Container>
    && (std::ranges::output_range<Container, std::byte>
        || std::ranges::output_range<Container, std::uint8_t>);
// clang-format on

template <binary_item_container Container>
inline auto parse_binary(parse_context &ctx,
                         Container &dest,
                         std::size_t const maxSize = SIZE_MAX) noexcept
        -> result<std::size_t>
{
    return detail::parse_blob<true>(ctx, dest, maxSize, type_code::binary);
}
template <binary_item_container Container>
inline auto parse_binary_finite(parse_context &ctx,
                                Container &dest,
                                std::size_t const maxSize = SIZE_MAX) noexcept
        -> result<std::size_t>
{
    return detail::parse_blob<false>(ctx, dest, maxSize, type_code::binary);
}

// clang-format off
template <typename Container>
concept text_item_container
    = detail::blob_output_container<Container>
    && (std::ranges::output_range<Container, char>
        || std::ranges::output_range<Container, char8_t>);
// clang-format on

template <text_item_container Container>
inline auto parse_text(parse_context &ctx,
                       Container &dest,
                       std::size_t const maxSize = SIZE_MAX) noexcept
        -> result<std::size_t>
{
    return detail::parse_blob<true>(ctx, dest, maxSize, type_code::text);
}
template <text_item_container Container>
inline auto parse_text_finite(parse_context &ctx,
                              Container &dest,
                              std::size_t const maxSize = SIZE_MAX) noexcept
        -> result<std::size_t>
{
    return detail::parse_blob<false>(ctx, dest, maxSize, type_code::text);
}

} // namespace dplx::dp

namespace dplx::dp
{

namespace detail
{

// clang-format off
template <typename Fn, typename Container>
concept subitem_parslet
    = requires(Fn &&decodeFn, parse_context &ctx, Container &dest, std::size_t const i)
    {
        { static_cast<Fn &&>(decodeFn)(ctx, dest, i) } noexcept
            -> tryable;
    };
// clang-format on

template <typename Container, typename DecodeElementFn>
inline auto
parse_indefinite_array_like(parse_context &ctx,
                            Container &dest,
                            std::size_t maxSize,
                            DecodeElementFn &&decodeElement) noexcept
        -> result<std::size_t>;

template <bool AllowIndefiniteEncoding,
          typename Container,
          typename DecodeElementFn>
inline auto parse_array_like(parse_context &ctx,
                             Container &dest,
                             std::size_t const maxSize,
                             type_code const expectedType,
                             DecodeElementFn &&decodeElement) noexcept
        -> result<std::size_t>
{
    result<item_head> headParseRx = dp::parse_item_head(ctx);
    if (headParseRx.has_error())
    {
        return static_cast<result<item_head> &&>(headParseRx).as_failure();
    }
    item_head const &head = headParseRx.assume_value();

    if (head.type != expectedType)
    {
        return errc::item_type_mismatch;
    }
    if (head.indefinite()) [[unlikely]]
    {
        if constexpr (AllowIndefiniteEncoding)
        {
            return detail::parse_indefinite_array_like(
                    ctx, dest, maxSize,
                    static_cast<DecodeElementFn &&>(decodeElement));
        }
        else
        {
            return errc::indefinite_item;
        }
    }
    if (ctx.in.input_size() >> (expectedType == type_code::map ? 1 : 0)
        < head.value)
    {
        // defend against amplification attacks exhausting main memory
        return errc::missing_data;
    }
    if (head.value > maxSize)
    {
        return errc::item_value_out_of_range;
    }

    auto const numElements = static_cast<std::size_t>(head.value);
    if constexpr (cncr::nothrow_tag_invocable<container_reserve_fn, Container &,
                                              std::size_t const>)
    {
        DPLX_TRY(container_reserve(dest, numElements));
    }

    for (std::size_t i = 0; i < numElements; ++i)
    {
        std::size_t const v = i;
        DPLX_TRY(static_cast<DecodeElementFn &&>(decodeElement)(ctx, dest, v));
    }
    return numElements;
}

template <typename Container, typename DecodeElementFn>
inline auto
parse_indefinite_array_like(parse_context &ctx,
                            Container &dest,
                            std::size_t const maxSize,
                            DecodeElementFn &&decodeElement) noexcept
        -> result<std::size_t>
{
    std::size_t i = 0;
    for (;; ++i)
    {
        DPLX_TRY(ctx.in.require_input(1U));
        if (*ctx.in.data() == static_cast<std::byte>(type_code::special_break))
        {
            ctx.in.discard_buffered(1U);
            break;
        }
        // overflow prevention
        if (i > maxSize || i == SIZE_MAX) [[unlikely]]
        {
            return errc::item_value_out_of_range;
        }

        std::size_t const v = i;
        DPLX_TRY(static_cast<DecodeElementFn &&>(decodeElement)(ctx, dest, v));
    }
    return i;
}

} // namespace detail

template <typename Container,
          detail::subitem_parslet<Container> DecodeElementFn>
inline auto parse_array(parse_context &ctx,
                        Container &dest,
                        DecodeElementFn &&decodeElement) -> result<std::size_t>

{
    return detail::parse_array_like<true, Container, DecodeElementFn>(
            ctx, dest, SIZE_MAX, type_code::array,
            static_cast<DecodeElementFn &&>(decodeElement));
}
template <typename Container,
          detail::subitem_parslet<Container> DecodeElementFn>
inline auto parse_array(parse_context &ctx,
                        Container &dest,
                        std::size_t const maxSize,
                        DecodeElementFn &&decodeElement) -> result<std::size_t>

{
    return detail::parse_array_like<true, Container, DecodeElementFn>(
            ctx, dest, maxSize, type_code::array,
            static_cast<DecodeElementFn &&>(decodeElement));
}
template <typename Container,
          detail::subitem_parslet<Container> DecodeElementFn>
inline auto parse_array_finite(parse_context &ctx,
                               Container &dest,
                               DecodeElementFn &&decodeElement)
        -> result<std::size_t>

{
    return detail::parse_array_like<false, Container, DecodeElementFn>(
            ctx, dest, SIZE_MAX, type_code::array,
            static_cast<DecodeElementFn &&>(decodeElement));
}
template <typename Container,
          detail::subitem_parslet<Container> DecodeElementFn>
inline auto parse_array_finite(parse_context &ctx,
                               Container &dest,
                               std::size_t const maxSize,
                               DecodeElementFn &&decodeElement)
        -> result<std::size_t>

{
    return detail::parse_array_like<false, Container, DecodeElementFn>(
            ctx, dest, maxSize, type_code::array,
            static_cast<DecodeElementFn &&>(decodeElement));
}

template <typename Container,
          detail::subitem_parslet<Container> DecodeElementFn>
inline auto parse_map(parse_context &ctx,
                      Container &dest,
                      DecodeElementFn &&decodeElement) -> result<std::size_t>
{
    return detail::parse_array_like<true, Container, DecodeElementFn>(
            ctx, dest, SIZE_MAX, type_code::map,
            static_cast<DecodeElementFn &&>(decodeElement));
}
template <typename Container,
          detail::subitem_parslet<Container> DecodeElementFn>
inline auto parse_map(parse_context &ctx,
                      Container &dest,
                      std::size_t const maxSize,
                      DecodeElementFn &&decodeElement) -> result<std::size_t>

{
    return detail::parse_array_like<true, Container, DecodeElementFn>(
            ctx, dest, maxSize, type_code::map,
            static_cast<DecodeElementFn &&>(decodeElement));
}
template <typename Container,
          detail::subitem_parslet<Container> DecodeElementFn>
inline auto parse_map_finite(parse_context &ctx,
                             Container &dest,
                             DecodeElementFn &&decodeElement)
        -> result<std::size_t>

{
    return detail::parse_array_like<false, Container, DecodeElementFn>(
            ctx, dest, SIZE_MAX, type_code::map,
            static_cast<DecodeElementFn &&>(decodeElement));
}
template <typename Container,
          detail::subitem_parslet<Container> DecodeElementFn>
inline auto parse_map_finite(parse_context &ctx,
                             Container &dest,
                             std::size_t const maxSize,
                             DecodeElementFn &&decodeElement)
        -> result<std::size_t>

{
    return detail::parse_array_like<false, Container, DecodeElementFn>(
            ctx, dest, maxSize, type_code::map,
            static_cast<DecodeElementFn &&>(decodeElement));
}

} // namespace dplx::dp
