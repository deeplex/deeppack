
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

#include <dplx/cncr/math_supplement.hpp>

#include <dplx/dp/config.hpp>
#include <dplx/dp/detail/bit.hpp>
#include <dplx/dp/detail/item_size.hpp>
#include <dplx/dp/detail/utils.hpp>
#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/items/type_code.hpp>
#include <dplx/dp/stream.hpp>

namespace dplx::dp::detail
{

template <typename T>
inline auto store_var_uint_ct(std::byte *dest,
                              T const value,
                              std::byte const category) noexcept -> int
{
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    if (value <= inline_value_max)
    {
        dest[0] = category | static_cast<std::byte>(value);
        return 1;
    }

    auto const lastSetBitIndex
            = static_cast<unsigned int>(detail::find_last_set_bit(value));
    int const bytePowerP2 = detail::find_last_set_bit(lastSetBitIndex);

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    dest[0] = category | static_cast<std::byte>(24 + bytePowerP2 - 2);

    auto const bitSize = 2 << bytePowerP2;
    auto const byteSize = bitSize >> 3;

    T const encoded = value << (digits_v<T> - bitSize);
    detail::store(dest + 1, encoded);

    return byteSize + 1;

    // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
}

template <typename T>
inline auto store_var_uint_branching(std::byte *dest,
                                     T const value,
                                     std::byte const category) noexcept -> int
{
    // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    if (value <= inline_value_max)
    {
        dest[0] = category | static_cast<std::byte>(value);
        return 1;
    }
    if (value <= 0xFFU)
    {
        dest[0] = category | std::byte{24U};
        dest[1] = static_cast<std::byte>(value);
        return 2;
    }
    if (value <= 0xFFFFU)
    {
        dest[0] = category | std::byte{25U};
        detail::store(dest + 1, static_cast<std::uint16_t>(value));
        return 3;
    }
    if (value <= 0xFFFF'FFFFU)
    {
        dest[0] = category | std::byte{26U};
        detail::store(dest + 1, static_cast<std::uint32_t>(value));
        return 5;
    }
    // else
    // {
    dest[0] = category | std::byte{27U};
    detail::store(dest + 1, static_cast<std::uint64_t>(value));
    return 9;
    // }
    // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    // NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
}

template <typename T>
inline auto store_var_uint(std::byte *dest,
                           T const value,
                           std::byte const category) noexcept -> int
{
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    static_assert(sizeof(T) <= 8U);
    static_assert(std::is_unsigned_v<T>);

#if !DPLX_DP_USE_BRANCHING_INTEGER_ENCODER

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    if constexpr (sizeof(T) <= 4U)
    {
        return detail::store_var_uint_ct(
                dest, static_cast<std::uint32_t>(value), category);
    }
    else
    {
        return detail::store_var_uint_ct(
                dest, static_cast<std::uint64_t>(value), category);
    }

#else

    return detail::store_var_uint_branching(dest, value, category);

#endif
}

} // namespace dplx::dp::detail

namespace dplx::dp
{

template <cncr::unsigned_integer T>
constexpr auto additional_information_size(T const value) noexcept
        -> unsigned int
{
    return detail::var_uint_encoded_size(value);
}

template <output_stream Stream>
class item_emitter
{
public:
    template <typename T>
    static inline auto var_uint_encoded_size(T const value) noexcept
            -> std::size_t
    {
        return static_cast<std::size_t>(
                detail::var_uint_encoded_size<T>(value));
    }

    template <typename T>
    static inline auto integer(Stream &outStream, T const value) -> result<void>
    {
        if constexpr (std::is_signed_v<T>)
        {
            using uvalue_type = std::make_unsigned_t<T>;
            auto const signmask = static_cast<uvalue_type>(
                    value >> (detail::digits_v<uvalue_type> - 1U));
            // complement negatives
            uvalue_type const uvalue
                    = signmask ^ static_cast<uvalue_type>(value);

            std::byte const category
                    = static_cast<std::byte>(signmask)
                    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
                    & std::byte{0b001'00000U};
            return item_emitter::encode_type_info(outStream, uvalue, category);
        }
        else
        {
            return item_emitter::encode_type_info(outStream, value,
                                                  to_byte(type_code::posint));
        }
    }

    template <typename T>
    static inline auto binary(Stream &outStream, T const byteSize)
            -> result<void>
    {
        return item_emitter::encode_type_info(outStream, byteSize,
                                              to_byte(type_code::binary));
    }
    template <typename T>
    static inline auto u8string(Stream &outStream, T const numCodeUnits)
            -> result<void>
    {
        return item_emitter::encode_type_info(outStream, numCodeUnits,
                                              to_byte(type_code::text));
    }
    template <typename T>
    static inline auto array(Stream &outStream, T const numElements)
            -> result<void>
    {
        return item_emitter::encode_type_info(outStream, numElements,
                                              to_byte(type_code::array));
    }
    static inline auto array_indefinite(Stream &outStream) -> result<void>
    {
        return item_emitter::encode_indefinite_type(outStream,
                                                    to_byte(type_code::array));
    }
    template <typename T>
    static inline auto map(Stream &outStream, T const numKeyValuePairs)
            -> result<void>
    {
        return item_emitter::encode_type_info(outStream, numKeyValuePairs,
                                              to_byte(type_code::map));
    }
    static inline auto map_indefinite(Stream &outStream) -> result<void>
    {
        return item_emitter::encode_indefinite_type(outStream,
                                                    to_byte(type_code::map));
    }
    static inline auto tag(Stream &outStream,
                           std::uint_least64_t const tagValue) -> result<void>
    {
        return item_emitter::encode_type_info(outStream, tagValue,
                                              to_byte(type_code::tag));
    }

    static inline auto boolean(Stream &outStream, bool const value)
            -> result<void>
    {
        DPLX_TRY(auto &&writeLease, write(outStream, 1));

        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        std::ranges::data(writeLease)[0]
                = to_byte(type_code::bool_false)
                | std::byte{static_cast<std::uint8_t>(value)};

        if constexpr (lazy_output_stream<Stream>)
        {
            DPLX_TRY(commit(outStream, writeLease));
        }
        return success();
    }
    static inline auto float_half(Stream &outStream, std::uint16_t const raw)
            -> result<void>
    {
        DPLX_TRY(auto &&writeLease, write(outStream, 1 + sizeof(raw)));

        auto const out = std::ranges::data(writeLease);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        out[0] = to_byte(type_code::float_half);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        std::memcpy(out + 1, &raw, sizeof(raw));

        if constexpr (lazy_output_stream<Stream>)
        {
            DPLX_TRY(commit(outStream, writeLease));
        }
        return success();
    }
    static inline auto float_single(Stream &outStream, float const value)
            -> result<void>
    {
        DPLX_TRY(auto &&writeLease, write(outStream, 1 + sizeof(value)));

        auto const out = std::ranges::data(writeLease);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        out[0] = to_byte(type_code::float_single);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        detail::store(out + 1, value);

        if constexpr (lazy_output_stream<Stream>)
        {
            DPLX_TRY(commit(outStream, writeLease));
        }
        return success();
    }
    static inline auto float_double(Stream &outStream, double const value)
            -> result<void>
    {
        DPLX_TRY(auto &&writeLease, write(outStream, 1 + sizeof(value)));

        auto const out = std::ranges::data(writeLease);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        out[0] = to_byte(type_code::float_double);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        detail::store(out + 1, value);

        if constexpr (lazy_output_stream<Stream>)
        {
            DPLX_TRY(commit(outStream, writeLease));
        }
        return success();
    }
    static inline auto null(Stream &outStream) -> result<void>
    {
        DPLX_TRY(auto &&writeLease, write(outStream, 1));

        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        std::ranges::data(writeLease)[0] = to_byte(type_code::null);

        if constexpr (lazy_output_stream<Stream>)
        {
            DPLX_TRY(commit(outStream, writeLease));
        }
        return success();
    }
    static inline auto undefined(Stream &outStream) -> result<void>
    {
        DPLX_TRY(auto &&writeLease, write(outStream, 1));

        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        std::ranges::data(writeLease)[0] = to_byte(type_code::undefined);

        if constexpr (lazy_output_stream<Stream>)
        {
            DPLX_TRY(commit(outStream, writeLease));
        }
        return success();
    }
    static inline auto break_(Stream &outStream) -> result<void>
    {
        DPLX_TRY(auto &&writeLease, write(outStream, 1));

        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        std::ranges::data(writeLease)[0] = to_byte(type_code::special_break);

        if constexpr (lazy_output_stream<Stream>)
        {
            DPLX_TRY(commit(outStream, writeLease));
        }
        return success();
    }

private:
    static inline auto encode_indefinite_type(Stream &outStream,
                                              std::byte const category)
            -> result<void>
    {
        DPLX_TRY(auto &&writeLease, write(outStream, 1));

        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        std::ranges::data(writeLease)[0]
                // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
                = category | std::byte{0b000'11111U};

        if constexpr (lazy_output_stream<Stream>)
        {
            DPLX_TRY(commit(outStream, writeLease));
        }
        return success();
    }

    template <typename T>
    static inline auto
    encode_type_info(Stream &outStream, T const value, std::byte const category)
            -> result<void>
    {
        if (auto maybeWriteLease = write(outStream, detail::var_uint_max_size);
            oc::try_operation_has_value(maybeWriteLease))
            DPLX_ATTR_LIKELY
            {
                auto &&writeLease = oc::try_operation_extract_value(
                        std::move(maybeWriteLease));
                auto const byteSize = detail::store_var_uint(
                        std::ranges::data(writeLease), value, category);

                DPLX_TRY(commit(outStream, writeLease,
                                static_cast<std::size_t>(byteSize)));
                return success();
            }
        else
        {
            if (result<void> failure
                = oc::try_operation_return_as(std::move(maybeWriteLease));
                failure.assume_error() != errc::end_of_stream)
            {
                return failure;
            }

            return item_emitter::encode_type_info_recover_eos(outStream, value,
                                                              category);
        }
    }

    template <typename T>
    // NOLINTNEXTLINE(readability-function-cognitive-complexity)
    static auto encode_type_info_recover_eos(Stream &outStream,
                                             T const value,
                                             std::byte const category)
            -> result<void>
    {
        // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
        // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        if (value <= detail::inline_value_max)
        {
            DPLX_TRY(auto &&writeLease, write(outStream, 1));
            auto out = std::ranges::data(writeLease);
            out[0] = category | static_cast<std::byte>(value);

            if constexpr (lazy_output_stream<Stream>)
            {
                DPLX_TRY(commit(outStream, writeLease));
            }
        }
        else if (value <= 0xFFU)
        {
            DPLX_TRY(auto &&writeLease, write(outStream, 2));
            auto out = std::ranges::data(writeLease);
            out[0] = category | std::byte{24};
            out[1] = static_cast<std::byte>(value);

            if constexpr (lazy_output_stream<Stream>)
            {
                DPLX_TRY(commit(outStream, writeLease));
            }
        }
        else if (value <= 0xFFFFU)
        {
            DPLX_TRY(auto &&writeLease, write(outStream, 3));
            auto out = std::ranges::data(writeLease);
            out[0] = category | std::byte{25};
            detail::store(out + 1, static_cast<std::uint16_t>(value));

            if constexpr (lazy_output_stream<Stream>)
            {
                DPLX_TRY(commit(outStream, writeLease));
            }
        }
        else if (value <= 0xFFFF'FFFFU)
        {
            DPLX_TRY(auto &&writeLease, write(outStream, 5));
            auto out = std::ranges::data(writeLease);
            out[0] = category | std::byte{26};
            detail::store(out + 1, static_cast<std::uint32_t>(value));

            if constexpr (lazy_output_stream<Stream>)
            {
                DPLX_TRY(commit(outStream, writeLease));
            }
        }
        else
        {
            // we initially tried writing 9B and the value to be
            // written would completely occupy them, therefore we
            // reinstate the end of stream error
            return errc::end_of_stream;
        }
        return success();
        // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        // NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
    }
};

} // namespace dplx::dp
