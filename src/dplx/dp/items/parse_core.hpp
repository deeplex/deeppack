
// Copyright Henrik Steffen Gaßmann 2023
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstdint>

#include <dplx/dp/detail/utils.hpp>
#include <dplx/dp/items/parse_context.hpp>
#include <dplx/dp/items/type_code.hpp>
#include <dplx/dp/streams/input_buffer.hpp>

namespace dplx::dp
{

struct item_head
{
    enum class flag : std::uint8_t
    {
        none = 0b0000'0000,
        indefinite = 0b0000'0001,
    };

    type_code type;
    flag flags;
    // std::uint16_t padding;
    std::uint32_t encoded_length;
    std::uint64_t value;

    [[nodiscard]] constexpr auto indefinite() const noexcept -> bool
    {
        return (static_cast<std::uint8_t>(flags)
                & static_cast<std::uint8_t>(flag::indefinite))
            != 0U;
    }
    constexpr void make_indefinite() noexcept
    {
        flags = static_cast<item_head::flag>(
                static_cast<std::uint8_t>(flags)
                | static_cast<std::uint8_t>(flag::indefinite));
    }

    [[nodiscard]] constexpr auto is_special_break() const noexcept -> bool
    {
        return type == type_code::special
            && (static_cast<std::uint8_t>(flags)
                & static_cast<std::uint8_t>(flag::indefinite))
                       != 0U;
    }

    friend inline constexpr auto operator==(item_head const &,
                                            item_head const &) noexcept -> bool
            = default;
};
#if !NDEBUG
static_assert(std::is_trivial_v<item_head>);
static_assert(std::is_standard_layout_v<item_head>);
static_assert(std::is_aggregate_v<item_head>);
#endif

} // namespace dplx::dp

namespace dplx::dp::detail
{

inline constexpr std::uint8_t item_type_mask = 0b111'00000U;
inline constexpr std::uint8_t item_inline_info_mask = 0b000'11111U;
inline constexpr unsigned item_var_int_coding_threshold = 27U;

inline auto parse_item_head_speculative(parse_context &ctx) noexcept
        -> result<item_head>
{
    std::byte const *const encoded = ctx.in.data();
    result<item_head> rx(item_head{
            .type = static_cast<type_code>(static_cast<std::uint8_t>(*encoded)
                                           & item_type_mask),
            .flags = item_head::flag::none,
            .encoded_length = 1,
            .value
            = static_cast<std::uint64_t>(static_cast<std::uint8_t>(*encoded)
                                         & item_inline_info_mask),
    });
    item_head &info = rx.assume_value();

    if (info.value <= inline_value_max)
    {
        // this is always well formed
    }
    else if (info.value <= item_var_int_coding_threshold) [[likely]]
    {
        auto const sizeBytesPower = static_cast<signed char>(
                static_cast<unsigned>(info.value) - (inline_value_max + 1U));
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        auto const encodedValue = detail::load<std::uint64_t>(encoded + 1);

        // 8B value => shift by  0 (0b00'0000)
        // 4B value => shift by 32 (0b10'0000)
        // 2B value => shift by 48 (0b11'0000)
        // 1B value => shift by 56 (0b11'1000)
        constexpr unsigned varLenPattern = 0b0011'1000U;
        unsigned char const varLenShift = (varLenPattern << sizeBytesPower)
                                        & (digits_v<std::uint64_t> - 1U);

        info.encoded_length = 1U + (1U << sizeBytesPower);
        info.value = encodedValue >> varLenShift;
    }
    // the one value which may also be well formed
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    else if (info.value == 31U
             // every type with one of these bits set has a meaningful value,
             // except for tag
             // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
             && !((static_cast<unsigned char>(info.type) & 0b110'00000U) == 0U
                  || info.type == type_code::tag))
    {
        info.make_indefinite();
    }
    else // 27 < addInfo < 31 || indefinite integer or tag
    {
        rx = errc::invalid_additional_information;
    }
    if (rx.has_value())
    {
        ctx.in.discard_buffered(info.encoded_length);
    }
    return rx;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
inline auto parse_item_head_safe(parse_context &ctx) noexcept
        -> result<item_head>
{
    assert(!ctx.in.empty());
    std::byte const indicator = *ctx.in.data();
    result<item_head> rx(item_head{
            .type = static_cast<type_code>(static_cast<std::uint8_t>(indicator)
                                           & item_type_mask),
            .flags = item_head::flag::none,
            .encoded_length = 1,
            .value
            = static_cast<std::uint64_t>(static_cast<std::uint8_t>(indicator)
                                         & item_inline_info_mask),
    });
    item_head &info = rx.assume_value();

    if (info.value <= inline_value_max)
    {
        // this is always well formed
    }
    else if (info.value <= item_var_int_coding_threshold) [[likely]]
    {
        auto const bytePower = info.value - (inline_value_max + 1U);
        info.encoded_length = 1U + (1U << bytePower);

        if (result<void> requireInputRx
            = ctx.in.require_input(info.encoded_length);
            requireInputRx.has_value())
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            std::byte const *const payload = ctx.in.data() + 1;
            switch (bytePower)
            {
            case 0U:
                info.value = static_cast<std::uint64_t>(*payload);
                break;
            case 1U:
                info.value = detail::load<std::uint16_t>(payload);
                break;
            case 2U:
                info.value = detail::load<std::uint32_t>(payload);
                break;
            case 3U:
                info.value = detail::load<std::uint64_t>(payload);
                break;
            }
        }
        else
        {
            rx = static_cast<result<void> &&>(requireInputRx).as_failure();
        }
    }
    // the one value which may also be well formed
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    else if (info.value == 31U
             // every type with one of these bits set has a meaningful value,
             // except for tag
             // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
             && !((static_cast<unsigned char>(info.type) & 0b110'00000U) == 0U
                  || info.type == type_code::tag))

    {
        info.make_indefinite();
    }
    else // 27 < addInfo < 31 || indefinite integer or tag
    {
        rx = errc::invalid_additional_information;
    }
    if (rx.has_value())
    {
        ctx.in.discard_buffered(info.encoded_length);
    }
    return rx;
}

} // namespace dplx::dp::detail

namespace dplx::dp
{

inline auto parse_item_head(parse_context &ctx) noexcept -> result<item_head>
{
    if (ctx.in.empty())
    {
        DPLX_TRY(ctx.in.require_input(1U));
    }
    if (ctx.in.size() >= detail::var_uint_max_size)
    {
        return detail::parse_item_head_speculative(ctx);
    }
    return detail::parse_item_head_safe(ctx);
}

} // namespace dplx::dp
