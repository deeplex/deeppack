
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>

#include <ranges>
#include <type_traits>

#include <dplx/dp/type_code.hpp>
#include <dplx/dp/utils.hpp>

namespace dplx::dp
{

// #TODO evaluate whether type_encoder can be made stream agnostic in a clean way
template <typename Stream> // #conceptify
class type_encoder
{
    using write_proxy = typename Stream::write_proxy;
    static constexpr int var_uint_max_size = 9;

    static constexpr int inline_value_max = 23;

public:
    template <typename T>
    static inline auto var_uint_size_of(T const value) noexcept -> std::size_t
    {
        return 1 + !(value <= inline_value_max) +
               !detail::fits_storage<std::uint8_t>(value) +
               !detail::fits_storage<std::uint16_t>(value) * 2 +
               !detail::fits_storage<std::uint32_t>(value) * 4;
    }

    template <typename T>
    static inline void integer(Stream &ctx, T const value)
    {
        if constexpr (std::is_signed_v<T>)
        {
            using uvalue_type = std::make_unsigned_t<T>;
            static constexpr auto valueDigits =
                std::numeric_limits<uvalue_type>::digits;
            auto const signmask =
                static_cast<uvalue_type>(value >> (valueDigits - 1));
            uvalue_type const uvalue = signmask ^ value; // complement negatives

            std::byte const category =
                static_cast<std::byte>(signmask) & std::byte{0b001'00000};
            type_encoder::encode_type_info(ctx, category, uvalue);
        }
        else
        {
            auto writeLease = ctx.write(var_uint_max_size);
            auto const byteSize = type_encoder::encode_uint(writeLease, value);
            writeLease.shrink(byteSize);
        }
    }

    template <typename T>
    static inline void binary(Stream &ctx, T const byteSize)
    {
        type_encoder::encode_type_info(
            ctx, to_byte(type_code::binary), byteSize);
    }
    template <typename T>
    static inline void u8string(Stream &ctx, T const numCodeUnits)
    {
        type_encoder::encode_type_info(
            ctx, to_byte(type_code::text), numCodeUnits);
    }
    template <typename T>
    static inline void array(Stream &ctx, T const numElements)
    {
        type_encoder::encode_type_info(
            ctx, to_byte(type_code::array), numElements);
    }
    template <typename T>
    static inline void map(Stream &ctx, T const numKeyValuePairs)
    {
        type_encoder::encode_type_info(
            ctx, to_byte(type_code::map), numKeyValuePairs);
    }
    static inline void tag(Stream &ctx, std::uint_least64_t const tagValue)
    {
        type_encoder::encode_type_info(ctx, to_byte(type_code::tag), tagValue);
    }

    static inline void boolean(Stream &ctx, bool const value)
    {
        auto writeLease = ctx.write(1);
        std::ranges::data(writeLease)[0] |=
            to_byte(type_code::bool_false) |
            std::byte{static_cast<std::uint8_t>(value)};
    }
    static inline void float_half(Stream &ctx, std::uint16_t const bytes)
    {
        auto writeLease = ctx.write(1 + sizeof(bytes));
        auto const out = std::ranges::data(writeLease);
        out[0] |= to_byte(type_code::float_half);
        std::memcpy(out + 1, &bytes, sizeof(bytes));
    }
    static inline void float_single(Stream &ctx, float const value)
    {
        auto writeLease = ctx.write(1 + sizeof(value));
        auto const out = std::ranges::data(writeLease);
        out[0] |= to_byte(type_code::float_single);
        detail::store(out + 1, value);
    }
    static inline void float_double(Stream &ctx, double const value)
    {
        auto writeLease = ctx.write(1 + sizeof(value));
        auto const out = std::ranges::data(writeLease);
        out[0] |= to_byte(type_code::float_double);
        detail::store(out + 1, value);
    }
    static inline void null(Stream &ctx)
    {
        auto writeLease = ctx.write(1);
        std::ranges::data(writeLease)[0] |= to_byte(type_code::null);
    }
    static inline void undefined(Stream &ctx)
    {
        auto writeLease = ctx.write(1);
        std::ranges::data(writeLease)[0] |= to_byte(type_code::undefined);
    }
    static inline void
    stop(Stream &ctx) // #TODO resolve naming inconsistency with type_code
    {
        auto writeLease = ctx.write(1);
        std::ranges::data(writeLease)[0] |= to_byte(type_code::special_break);
    }

private:
    template <typename T>
    static inline void
    encode_type_info(Stream &ctx, std::byte const category, T const value)
    {
        auto writeLease = ctx.write(var_uint_max_size);
        auto const byteSize = type_encoder::encode_uint(writeLease, value);

        std::ranges::data(writeLease)[0] |= category;
        writeLease.shrink(byteSize);
    }

    template <typename T> // #conceptify
    static inline auto encode_uint(write_proxy &writeLease,
                                   T const value) noexcept -> int
    {
        static_assert(std::is_unsigned_v<T>);

        int const cat0 = !(value <= 23);
        int const cat1 = !detail::fits_storage<std::uint8_t>(value);
        int const cat2 = !detail::fits_storage<std::uint16_t>(value);
        int const cat3 = !detail::fits_storage<std::uint32_t>(value);
        auto const byteSize = 1 + cat0 + cat1 + cat2 * 2 + cat3 * 4;

        auto out = std::ranges::data(writeLease);

        out[0] = std::byte{24} |
                 std::byte{static_cast<std::uint8_t>(cat1 + cat2 + cat3)};

        static constexpr int digitDistance =
            detail::unsigned_digit_distance_v<std::uint64_t, T>;

        T const encoded =
            value << (56 - digitDistance - cat1 * 8 - cat2 * 16 - cat3 * 32);
        detail::store(out + cat0, encoded);

        return byteSize;
    }
};

} // namespace dplx::dp
