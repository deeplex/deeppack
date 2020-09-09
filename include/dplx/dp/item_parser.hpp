
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>
#include <cstdint>

#include <ranges>
#include <type_traits>

#include <dplx/dp/detail/bit.hpp>
#include <dplx/dp/detail/utils.hpp>
#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/stream.hpp>
#include <dplx/dp/type_code.hpp>

namespace dplx::dp::detail
{

enum class decode_errc : std::uint8_t
{
    nothing = 0,
    invalid_additional_information =
        static_cast<std::uint8_t>(errc::invalid_additional_information),
};

struct item_info
{
    decode_errc code;
    std::uint8_t type;
    std::int32_t encoded_length;
    std::uint64_t value;

    bool indefinite() const noexcept
    {
        return (type & 0b000'00001) != 0;
    }
    void make_indefinite() noexcept
    {
        type |= 0b000'00001;
    }

    constexpr bool operator==(item_info const &) const noexcept = default;
};
static_assert(std::is_trivial_v<item_info>);
static_assert(std::is_standard_layout_v<item_info>);

inline auto parse_item_info_speculative(std::byte const *const encoded)
    -> item_info
{
    item_info info{
        .code = decode_errc::nothing,
        .type = static_cast<std::uint8_t>(*encoded & std::byte{0b111'00000}),
        .encoded_length = 1,
        .value = static_cast<std::uint64_t>(*encoded & std::byte{0b000'11111}),
    };

    if (info.value < (inline_value_max + 1))
    {
        // this is always well formed
    }
    else if (info.value <= 27)
        DPLX_ATTR_LIKELY
        {
            auto const sizeBytesPower =
                static_cast<signed char>(info.value - (inline_value_max + 1));
            auto const encodedValue = detail::load<std::uint64_t>(encoded + 1);

            // 8B value => shift by  0 (0b00'0000)
            // 4B value => shift by 32 (0b10'0000)
            // 2B value => shift by 48 (0b11'0000)
            // 1B value => shift by 56 (0b11'1000)
            unsigned char const varLenShift =
                (0b0011'1000 << sizeBytesPower) & 63;

            info.encoded_length = 1 + (1 << sizeBytesPower);
            info.value = encodedValue >> varLenShift;
        }
    else if (info.value == 31 &&
             !((info.type & 0b110'00000) == 0 ||
               info.type == static_cast<std::uint16_t>(type_code::tag)))
    {
        info.make_indefinite();
    }
    else // 27 < addInfo < 31 || indefinite integer or tag
    {
        info.code = decode_errc::invalid_additional_information;
        info.type = 0;
        info.value = 0;
    }
    return info;
}

template <typename Stream>
inline auto parse_item_info_safe(Stream &stream) -> result<item_info>
{
    DPLX_TRY(auto &&indicatorProxy, dp::read(stream, 1));
    std::byte const *const indicator = std::ranges::data(indicatorProxy);

    item_info info{
        .code = decode_errc::nothing,
        .type = static_cast<std::uint8_t>(*indicator & std::byte{0b111'00000}),
        .encoded_length = 1,
        .value =
            static_cast<std::uint64_t>(*indicator & std::byte{0b000'11111}),
    };

    if constexpr (lazy_input_stream<Stream>)
    {
        DPLX_TRY(dp::consume(stream, indicatorProxy));
    }

    if (info.value < (inline_value_max + 1))
    {
        // this is always well formed
    }
    else if (info.value <= 27)
        DPLX_ATTR_LIKELY
        {
            auto const bytePower = info.value - (inline_value_max + 1);
            auto xlength = (1 << bytePower);
            info.encoded_length = 1 + xlength;

            // read expects a std::size_t value;
            // and we can't just repeat the shift, because MSVC starts crying
            // about extending a 32bit shift into a 64bit variable
            std::size_t const length = static_cast<std::size_t>(xlength);
            DPLX_TRY(auto &&payloadProxy, dp::read(stream, length));

            std::byte const *const payload = std::ranges::data(payloadProxy);
            switch (bytePower)
            {
            case 0:
                info.value = static_cast<std::uint64_t>(*payload);
                break;
            case 1:
                info.value = load<std::uint16_t>(payload);
                break;
            case 2:
                info.value = load<std::uint32_t>(payload);
                break;
            case 3: // this case can only be reached if stream misbehaves
                info.value = load<std::uint64_t>(payload);
                break;
            }

            if constexpr (lazy_input_stream<Stream>)
            {
                DPLX_TRY(dp::consume(stream, payloadProxy));
            }
        }
    else if (info.value == 31 &&
             !((info.type & 0b110'00000) == 0 ||
               info.type == static_cast<std::uint16_t>(type_code::tag)))

    {
        info.type |= 0b000'00001;
    }
    else // 27 < addInfo < 31 || indefinite integer or tag

    {
        return errc::invalid_additional_information;
    }
    return info;
}

template <input_stream Stream>
inline auto parse_item_info(Stream &stream) -> result<item_info>
{
    if (auto readRx = dp::read(stream, var_uint_max_size);
        oc::try_operation_has_value(readRx))
        DPLX_ATTR_LIKELY
        {
            auto &&readProxy =
                oc::try_operation_extract_value(std::move(readRx));
            std::byte const *const memory = std::ranges::data(readProxy);
            auto const info = dp::detail::parse_item_info_speculative(memory);

            auto const consumedBytes =
                static_cast<std::size_t>(info.encoded_length);
            DPLX_TRY(dp::consume(stream, readProxy, consumedBytes));
            if (info.code == decode_errc::nothing)
            {
                return info;
            }
            else
            {
                return static_cast<errc>(info.code);
            }
        }
    else
    {
        result<item_info> rx = oc::try_operation_return_as(std::move(readRx));

        if (rx.assume_error() == errc::end_of_stream)
        {
            return dp::detail::parse_item_info_safe(stream);
        }
        return rx;
    }
}

template <input_stream Stream>
inline auto skip_item(Stream &stream) -> result<void>
{
    (void)stream;
    return success(); // #TODO #NotImplemented
}

} // namespace dplx::dp::detail
