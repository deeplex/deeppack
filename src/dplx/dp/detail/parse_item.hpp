
// Copyright Henrik Steffen Gaßmann 2020-2021
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include <dplx/cncr/misc.hpp>
#include <dplx/predef/compiler.h>

#include <dplx/dp/customization.hpp>
#include <dplx/dp/detail/type_utils.hpp>
#include <dplx/dp/detail/utils.hpp>
#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/stream.hpp>
#include <dplx/dp/type_code.hpp>

namespace dplx::dp
{

struct item_info
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

    [[nodiscard]] bool indefinite() const noexcept
    {
        return cncr::to_underlying(flags)
             & cncr::to_underlying(flag::indefinite);
    }
    void make_indefinite() noexcept
    {
        flags = static_cast<item_info::flag>(
                cncr::to_underlying(flags)
                | cncr::to_underlying(flag::indefinite));
    }

    [[nodiscard]] bool is_special_break() const noexcept
    {
        return type == type_code::special
            && cncr::to_underlying(flags)
                       & cncr::to_underlying(flag::indefinite);
    }

    friend inline constexpr bool
    operator==(item_info const &, item_info const &) noexcept = default;
};
static_assert(std::is_trivial_v<item_info>);
static_assert(std::is_standard_layout_v<item_info>);

} // namespace dplx::dp

namespace dplx::dp::detail
{

inline auto parse_item_speculative(std::byte const *const encoded) noexcept
        -> result<item_info>
{
    item_info info{
            .type = static_cast<type_code>(*encoded & std::byte{0b111'00000}),
            .flags = item_info::flag::none,
            .encoded_length = 1,
            .value
            = static_cast<std::uint64_t>(*encoded & std::byte{0b000'11111}),
    };

    if (info.value <= inline_value_max)
    {
        // this is always well formed
    }
    else if (info.value <= 27)
        DPLX_ATTR_LIKELY
        {
            auto const sizeBytesPower = static_cast<signed char>(
                    info.value - (inline_value_max + 1));
            auto const encodedValue = detail::load<std::uint64_t>(encoded + 1);

            // 8B value => shift by  0 (0b00'0000)
            // 4B value => shift by 32 (0b10'0000)
            // 2B value => shift by 48 (0b11'0000)
            // 1B value => shift by 56 (0b11'1000)
            unsigned char const varLenShift
                    = (0b0011'1000 << sizeBytesPower) & 63;

            info.encoded_length = 1 + (1 << sizeBytesPower);
            info.value = encodedValue >> varLenShift;
        }
    else if (info.value == 31
             && !((static_cast<std::byte>(info.type) & std::byte{0b110'00000})
                          == std::byte{}
                  || info.type == type_code::tag))
    {
        info.make_indefinite();
    }
    else // 27 < addInfo < 31 || indefinite integer or tag
    {
        return errc::invalid_additional_information;
    }
    return info;
}

template <input_stream Stream>
inline auto parse_item_safe(Stream &inStream) noexcept(
        stream_traits<Stream>::nothrow_read_direct
                &&stream_traits<Stream>::nothrow_consume) -> result<item_info>
{
    DPLX_TRY(auto &&indicatorProxy, read(inStream, 1u));
    std::byte const *const indicator = std::ranges::data(indicatorProxy);

    dp::item_info info{
            .type = static_cast<type_code>(*indicator & std::byte{0b111'00000}),
            .flags = dp::item_info::flag::none,
            .encoded_length = 1,
            .value
            = static_cast<std::uint64_t>(*indicator & std::byte{0b000'11111}),
    };

    if (detail::inline_value_max < info.value && info.value <= 27)
        DPLX_ATTR_LIKELY
        {
            auto const bytePower = info.value - (detail::inline_value_max + 1);
            info.encoded_length = 1u + (1u << bytePower);

            // ensure we consume the whole item or nothing
            DPLX_TRY(consume(inStream, indicatorProxy, 0u));

            DPLX_TRY(auto &&payloadProxy,
                     read(inStream,
                          static_cast<std::size_t>(info.encoded_length)));

            std::byte const *payload = std::ranges::data(payloadProxy);
            payload += 1;
            switch (bytePower)
            {
            case 0:
                info.value = static_cast<std::uint64_t>(*payload);
                break;
            case 1:
                info.value = detail::load<std::uint16_t>(payload);
                break;
            case 2:
                info.value = detail::load<std::uint32_t>(payload);
                break;
            case 3: // this case can only be reached if inStream misbehaves
                info.value = detail::load<std::uint64_t>(payload);
                break;
            }

            if constexpr (lazy_input_stream<Stream>)
            {
                DPLX_TRY(consume(inStream, payloadProxy));
            }
            return info;
        }

    if constexpr (lazy_input_stream<Stream>)
    {
        DPLX_TRY(consume(inStream, indicatorProxy));
    }

    if (info.value <= detail::inline_value_max)
    {
        // this is always well formed
    }
    else if (info.value == 31
             && !((static_cast<std::byte>(info.type) & std::byte{0b110'00000})
                          == std::byte{}
                  || info.type == type_code::tag))

    {
        info.make_indefinite();
    }
    else // 27 < addInfo < 31 || indefinite integer or tag
    {
        return errc::invalid_additional_information;
    }
    return info;
}

template <input_stream Stream>
inline auto parse_item(Stream &stream) noexcept(
        stream_traits<Stream>::nothrow_read_direct
                &&stream_traits<Stream>::nothrow_consume) -> result<item_info>
{
    if (auto readRx = read(stream, var_uint_max_size);
        oc::try_operation_has_value(readRx))
        DPLX_ATTR_LIKELY
        {
            auto &&readProxy
                    = oc::try_operation_extract_value(std::move(readRx));
            std::byte const *const memory = std::ranges::data(readProxy);
            DPLX_TRY(auto &&info, detail::parse_item_speculative(memory));

            DPLX_TRY(consume(stream, readProxy,
                             static_cast<std::size_t>(info.encoded_length)));

            return info;
        }
    else
    {
        result<dp::item_info> rx
                = oc::try_operation_return_as(std::move(readRx));

        if (rx.assume_error() == errc::end_of_stream)
        {
            return detail::parse_item_safe(stream);
        }
        return rx;
    }
}

static inline auto load_iec559_half(std::uint16_t bits) noexcept -> double
{
    // IEC 60559:2011 half precision
    // 1bit sign | 5bit exponent | 10bit significand
    // 0x8000    | 0x7C00        | 0x3ff

    // #TODO check whether I got the endianess right
    unsigned int significand = bits & 0x3ffu;
    int exponent = (bits >> 10) & 0x1f;

    double value;
    if (exponent == 0) // zero | subnormal
    {
        value = std::ldexp(significand, -24);
    }
    else if (exponent != 0x1f) // normalized values
    {
        // 0x400 => implicit lead bit
        // 25 = 15 exponent bias + 10bit significand
        value = std::ldexp(significand + 0x400, exponent - 25);
    }
    else if (significand == 0)
    {
        value = std::numeric_limits<double>::infinity();
    }
    else
    {
        value = std::numeric_limits<double>::quiet_NaN();
    }
    // respect sign bit
    return (bits & 0x8000) == 0 ? value : -value;
}

} // namespace dplx::dp::detail
