
// Copyright Henrik Steffen Gaßmann 2023
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <bit>
#include <cmath>
#include <cstdint>

#include <dplx/dp/detail/bit.hpp>
#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/items/encoded_item_head_size.hpp>
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

        if (info.type == type_code::special
            && sizeBytesPower == 0
            // encoding type 7 (special) values [0..32] with two bytes is
            // forbidden as per RFC8949 section 3.3
            // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
            && info.value < 0x20) [[unlikely]]
        {
            rx = errc::invalid_additional_information;
        }
    }
    // the one value which may also be well formed
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    else if (info.value == 31U
             // every type with one of these bits set has a meaningful value,
             // except for tag
             // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
             && (static_cast<unsigned char>(info.type) & 0b110'00000U) != 0U
             && info.type != type_code::tag)
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
                if (info.type == type_code::special
                    // encoding type 7 (special) values [0..32] with two bytes
                    // is forbidden as per RFC8949 section 3.3
                    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
                    && info.value < 0x20) [[unlikely]]
                {
                    rx = errc::invalid_additional_information;
                }
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
             && (static_cast<unsigned char>(info.type) & 0b110'00000U) != 0U
             && info.type != type_code::tag)

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

inline auto expect_item_head(parse_context &ctx,
                             type_code const type,
                             std::uint64_t const value) noexcept -> result<void>
{
    DPLX_TRY(item_head const &head, dp::parse_item_head(ctx));

    if (head.type != type) [[unlikely]]
    {
        return errc::item_type_mismatch;
    }
    if (head.indefinite()) [[unlikely]]
    {
        if (type == type_code::special)
        {
            // note that this isn't ambiguous due to 0x1f being a reserved
            // special value
            // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
            if (value == 0x1fU)
            {
                return oc::success();
            }
            // unwanted special break.
            return errc::item_type_mismatch;
        }
        return errc::indefinite_item;
    }
    if (head.value != value) [[unlikely]]
    {
        return errc::item_value_out_of_range;
    }
    return oc::success();
}

template <detail::encodable_int T>
    requires std::signed_integral<T>
inline auto parse_integer(parse_context &ctx, T &outValue) noexcept
        -> result<void>
{
    auto parseRx = dp::parse_item_head(ctx);
    if (parseRx.has_error())
    {
        return static_cast<result<item_head> &&>(parseRx).as_failure();
    }
    item_head const &head = parseRx.assume_value();
    // DPLX_TRY(item_head const &head, dp::parse_item_head(ctx));

    if (head.type != type_code::posint && head.type != type_code::negint)
    {
        return errc::item_type_mismatch;
    }

    // fused check for both positive and negative integers
    // negative integers are encoded as (-1 -n)
    // therefore the largest representable additional
    // information value is the same as the smallest one
    // e.g. a signed 8bit two's complement min() is -128 which
    // would be encoded as (-1 -[n=127])
    if (head.value
        > static_cast<std::make_unsigned_t<T>>(std::numeric_limits<T>::max()))
    {
        return errc::item_value_out_of_range;
    }

    auto const signBit = static_cast<std::uint64_t>(head.type) << 58;
    auto const signExtended = static_cast<std::int64_t>(signBit) >> 63;
    auto const xorpad = static_cast<std::uint64_t>(signExtended);

    outValue = static_cast<T>(head.value ^ xorpad);
    return oc::success();
}

template <detail::encodable_int T>
    requires std::unsigned_integral<T>
inline auto parse_integer(parse_context &ctx,
                          T &outValue,
                          T limit = std::numeric_limits<T>::max()) noexcept
        -> result<void>
{
    auto parseRx = dp::parse_item_head(ctx);
    if (parseRx.has_error())
    {
        return static_cast<result<item_head> &&>(parseRx).as_failure();
    }
    item_head const &head = parseRx.assume_value();
    // DPLX_TRY(item_head const &head, dp::parse_item_head(ctx));

    if (head.type != type_code::posint)
    {
        return errc::item_type_mismatch;
    }
    if (head.value > limit)
    {
        return errc::item_value_out_of_range;
    }
    outValue = static_cast<T>(head.value);
    return oc::success();
}

inline auto parse_boolean(parse_context &ctx, bool &outValue) noexcept
        -> result<void>
{
    constexpr auto boolPattern = static_cast<unsigned>(type_code::bool_false);

    if (ctx.in.empty()) [[unlikely]]
    {
        DPLX_TRY(ctx.in.require_input(1U));
    }

    auto const value = static_cast<unsigned>(*ctx.in.data());
    auto const rolled = value - boolPattern;

    if (rolled > 1U) [[unlikely]]
    {
        return errc::item_type_mismatch;
    }
    outValue = rolled == 1U;
    ctx.in.discard_buffered(1U);
    return oc::success();
}

namespace detail
{

inline auto load_iec559_half(unsigned bits) noexcept -> double
{
    // IEC 60559:2011 half precision
    // 1bit sign | 5bit exponent | 10bit significand
    // 0x8000    | 0x7C00        | 0x3ff

    // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)

    unsigned const significand = bits & 0x3ffU;
    auto const exponent = static_cast<int>((bits >> 10) & 0x1fU);

    double value;      // NOLINT(cppcoreguidelines-init-variables)
    if (exponent == 0) // zero | subnormal
    {
        value = std::ldexp(significand, -24);
    }
    else if (exponent != 0x1F) // normalized values
    {
        // 0x400 => implicit lead bit
        // 25 = 15 exponent bias + 10bit significand
        value = std::ldexp(significand + 0x400U, exponent - 25);
    }
    else if (significand == 0U)
    {
        value = std::numeric_limits<double>::infinity();
    }
    else
    {
        value = std::numeric_limits<double>::quiet_NaN();
    }
    // respect sign bit
    return (bits & 0x8000U) == 0U ? value : -value;

    // NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
}

} // namespace detail

template <typename T>
inline auto parse_floating_point(parse_context &ctx, T &outValue) noexcept
        -> result<void>
{
    auto parseRx = dp::parse_item_head(ctx);
    if (parseRx.has_error())
    {
        return static_cast<result<item_head> &&>(parseRx).as_failure();
    }
    item_head const &head = parseRx.assume_value();

    if (head.type != type_code::special || head.encoded_length < 3U)
            [[unlikely]]
    {
        return errc::item_type_mismatch;
    }

    if (head.encoded_length == 1U + sizeof(std::uint64_t))
    {
        if constexpr (sizeof(T) >= sizeof(std::uint64_t))
        {
            outValue = std::bit_cast<double>(
                    static_cast<std::uint64_t>(head.value));
        }
        else
        {
            return errc::item_value_out_of_range;
        }
    }
    else if (head.encoded_length == 1U + sizeof(std::uint32_t))
    {
        if constexpr (sizeof(T) >= sizeof(std::uint32_t))
        {
            if constexpr (std::is_same_v<T, float>)
            {
                outValue = std::bit_cast<float>(
                        static_cast<std::uint32_t>(head.value));
            }
            else
            {
                outValue = static_cast<T>(std::bit_cast<float>(
                        static_cast<std::uint32_t>(head.value)));
            }
        }
        else
        {
            return errc::item_value_out_of_range;
        }
    }
    else
    {
        outValue = static_cast<T>(
                detail::load_iec559_half(static_cast<unsigned>(head.value)));
    }
    return dp::success();
}

} // namespace dplx::dp
