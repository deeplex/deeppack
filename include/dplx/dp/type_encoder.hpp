
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
#include <dplx/dp/type_code.hpp>
#include <dplx/dp/utils.hpp>

namespace dplx::dp::detail
{

inline constexpr int var_uint_max_size = 9;
inline constexpr unsigned int inline_value_max = 23;

#if !DEEPPACK_USE_BRANCHING_INTEGER_ENCODER

template <typename T>
inline auto store_var_uint_impl(std::byte *dest,
                                T const value,
                                std::byte const category) noexcept -> int
{
    if (value <= inline_value_max)
    {
        dest[0] = category | static_cast<std::byte>(value);
        return 1;
    }

    unsigned int const lastSetBitIndex = detail::find_last_set_bit(value);
    int const bytePowerP2 = detail::find_last_set_bit(lastSetBitIndex);

    dest[0] = category | static_cast<std::byte>(24 + bytePowerP2 - 2);

    auto const bitSize = 2 << bytePowerP2;
    auto const byteSize = bitSize >> 3;

    T const encoded = value << (digits_v<T> - bitSize);
    detail::store(dest + 1, encoded);

    return byteSize + 1;
}

template <typename T>
inline auto var_uint_encoded_size_impl(T const value) -> int
{
    if (value <= inline_value_max)
    {
        return 1;
    }
    unsigned int const lastSetBitIndex = detail::find_last_set_bit(value);
    unsigned int const bytePower =
        detail::find_last_set_bit(lastSetBitIndex) - 2;

    return 1 + (1 << bytePower);
}

#else

template <typename T>
inline auto store_var_uint_impl(std::byte *dest,
                                T const value,
                                std::byte const category) noexcept -> int
{
    if (value <= inline_value_max)
    {
        dest[0] = category | static_cast<std::byte>(value);
        return 1;
    }
    if (value <= 0xff)
    {
        dest[0] = category | std::byte{24};
        dest[1] = static_cast<std::byte>(value);
        return 2;
    }
    if (value <= 0xffff)
    {
        dest[0] = category | std::byte{25};
        detail::store(dest + 1, static_cast<std::uint16_t>(value));
        return 3;
    }
    if (value <= 0xffff'ffff)
    {
        dest[0] = category | std::byte{26};
        detail::store(dest + 1, static_cast<std::uint32_t>(value));
        return 5;
    }
    else
    {
        dest[0] = category | std::byte{27};
        detail::store(dest + 1, static_cast<std::uint64_t>(value));
        return 9;
    }
}

template <typename T>
inline auto var_uint_encoded_size_impl(T const value) -> int
{
    if (value <= inline_value_max)
    {
        return 1;
    }
    if (value <= 0xff)
    {
        return 2;
    }
    if (value <= 0xffff)
    {
        return 3;
    }
    if (value <= 0xffff'ffff)
    {
        return 5;
    }
    else
    {
        return 9;
    }
}

#endif

template <typename T>
inline auto store_var_uint(std::byte *dest,
                           T const value,
                           std::byte const category) noexcept -> int
{
    static_assert(sizeof(T) <= 8);
    static_assert(std::is_unsigned_v<T>);

    if constexpr (sizeof(T) <= 4)
    {
        return store_var_uint_impl(
            dest, static_cast<std::uint32_t>(value), category);
    }
    else
    {
        return store_var_uint_impl(
            dest, static_cast<std::uint64_t>(value), category);
    }
}

template <typename T>
inline auto var_uint_encoded_size(T const value) -> int
{
    static_assert(sizeof(T) <= 8);
    static_assert(std::is_unsigned_v<T>);

    if constexpr (sizeof(T) <= 4)
    {
        return var_uint_encoded_size_impl(static_cast<std::uint32_t>(value));
    }
    else
    {
        return var_uint_encoded_size_impl(static_cast<std::uint64_t>(value));
    }
}

} // namespace dplx::dp::detail

namespace dplx::dp
{

// #TODO evaluate whether type_encoder can be made stream agnostic in a clean
// way
template <typename Stream> // #conceptify
class type_encoder
{
    using write_proxy = typename Stream::write_proxy;

public:
    template <typename T>
    static inline auto var_uint_encoded_size(T const value) noexcept
        -> std::size_t
    {
        return static_cast<std::size_t>(detail::var_uint_encoded_size<T>(value));
    }

    template <typename T>
    static inline void integer(Stream &ctx, T const value)
    {
        if constexpr (std::is_signed_v<T>)
        {
            using uvalue_type = std::make_unsigned_t<T>;
            auto const signmask = static_cast<uvalue_type>(
                value >> (detail::digits_v<uvalue_type> - 1));
            uvalue_type const uvalue = signmask ^ value; // complement negatives

            std::byte const category =
                static_cast<std::byte>(signmask) & std::byte{0b001'00000};
            type_encoder::encode_type_info(ctx, uvalue, category);
        }
        else
        {
            type_encoder::encode_type_info(
                ctx, value, to_byte(type_code::posint));
        }
    }

    template <typename T>
    static inline void binary(Stream &ctx, T const byteSize)
    {
        type_encoder::encode_type_info(
            ctx, byteSize, to_byte(type_code::binary));
    }
    template <typename T>
    static inline void u8string(Stream &ctx, T const numCodeUnits)
    {
        type_encoder::encode_type_info(
            ctx, numCodeUnits, to_byte(type_code::text));
    }
    template <typename T>
    static inline void array(Stream &ctx, T const numElements)
    {
        type_encoder::encode_type_info(
            ctx, numElements, to_byte(type_code::array));
    }
    static inline void array_indefinite(Stream &ctx)
    {
        type_encoder::encode_indefinite_type(ctx, to_byte(type_code::array));
    }
    template <typename T>
    static inline void map(Stream &ctx, T const numKeyValuePairs)
    {
        type_encoder::encode_type_info(
            ctx, numKeyValuePairs, to_byte(type_code::map));
    }
    static inline void map_indefinite(Stream &ctx)
    {
        type_encoder::encode_indefinite_type(ctx, to_byte(type_code::map));
    }
    static inline void tag(Stream &ctx, std::uint_least64_t const tagValue)
    {
        type_encoder::encode_type_info(ctx, tagValue, to_byte(type_code::tag));
    }

    static inline void boolean(Stream &ctx, bool const value)
    {
        auto writeLease = ctx.write(1);
        std::ranges::data(writeLease)[0] =
            to_byte(type_code::bool_false) |
            std::byte{static_cast<std::uint8_t>(value)};
    }
    static inline void float_half(Stream &ctx, std::uint16_t const bytes)
    {
        auto writeLease = ctx.write(1 + sizeof(bytes));
        auto const out = std::ranges::data(writeLease);
        out[0] = to_byte(type_code::float_half);
        std::memcpy(out + 1, &bytes, sizeof(bytes));
    }
    static inline void float_single(Stream &ctx, float const value)
    {
        auto writeLease = ctx.write(1 + sizeof(value));
        auto const out = std::ranges::data(writeLease);
        out[0] = to_byte(type_code::float_single);
        detail::store(out + 1, value);
    }
    static inline void float_double(Stream &ctx, double const value)
    {
        auto writeLease = ctx.write(1 + sizeof(value));
        auto const out = std::ranges::data(writeLease);
        out[0] = to_byte(type_code::float_double);
        detail::store(out + 1, value);
    }
    static inline void null(Stream &ctx)
    {
        auto writeLease = ctx.write(1);
        std::ranges::data(writeLease)[0] = to_byte(type_code::null);
    }
    static inline void undefined(Stream &ctx)
    {
        auto writeLease = ctx.write(1);
        std::ranges::data(writeLease)[0] = to_byte(type_code::undefined);
    }
    static inline void break_(Stream &ctx)
    {
        auto writeLease = ctx.write(1);
        std::ranges::data(writeLease)[0] = to_byte(type_code::special_break);
    }

private:
    static inline void encode_indefinite_type(Stream &ctx,
                                              std::byte const category)
    {
        auto writeLease = ctx.write(1);
        std::ranges::data(writeLease)[0] = category | std::byte{0b000'11111};
    }

    template <typename T>
    static inline void
    encode_type_info(Stream &ctx, T const value, std::byte const category)
    {
        auto writeLease = ctx.write(detail::var_uint_max_size);
        auto const byteSize = detail::store_var_uint(
            std::ranges::data(writeLease), value, category);

        writeLease.shrink(byteSize);
    }
};

} // namespace dplx::dp
