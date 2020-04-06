
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

template <typename Stream> // #conceptify
class type_encoder
{
    using write_proxy = typename Stream::write_proxy;
    static constexpr int var_uint_max_size = 9;

public:
    template <typename T>
    static inline void encode_integer(Stream &ctx, T const value)
    {
        auto writeLease = ctx.write(var_uint_max_size);
        if constexpr (std::is_signed_v<T>)
        {
            using uvalue_type = std::make_unsigned_t<T>;
            static constexpr auto valueDigits =
                std::numeric_limits<uvalue_type>::digits;
            auto const signmask =
                static_cast<uvalue_type>(value >> (valueDigits - 1));
            uvalue_type const uvalue = signmask ^ value; // complement negatives

            auto const byteSize =
                type_encoder::encode_var_uint(writeLease, uvalue);

            std::ranges::data(writeLease)[0] |=
                static_cast<std::byte>(signmask) & std::byte{0b001'00000};

            writeLease.shrink(byteSize);
        }
        else
        {
            auto const byteSize =
                type_encoder::encode_var_uint(writeLease, value);
            writeLease.shrink(byteSize);
        }
    }

private:
    template <typename T> // #conceptify
    static inline auto encode_var_uint(write_proxy &writeLease,
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
