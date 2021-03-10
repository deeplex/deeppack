
// Copyright Henrik Steffen Gaßmann 2020.
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include <limits>
#include <ranges>
#include <type_traits>

#include <dplx/dp/concepts.hpp>
#include <dplx/dp/detail/type_utils.hpp>
#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/fwd.hpp>
#include <dplx/dp/item_parser.hpp>

namespace dplx::dp
{

// volatile types are not supported.
template <typename T, input_stream Stream>
class basic_decoder<volatile T, Stream>;
template <typename T, input_stream Stream>
class basic_decoder<volatile T const, Stream>;

template <integer T, input_stream Stream>
class basic_decoder<T, Stream>
{
    using limits = std::numeric_limits<T>;

public:
    auto operator()(Stream &inStream, T &dest) const -> result<void>
    {
        DPLX_TRY(auto &&info, detail::parse_item_info(inStream));
        if constexpr (std::is_unsigned_v<T>)
        {
            if (std::byte{info.type} != type_code::posint)
            {
                return errc::item_type_mismatch;
            }
            if constexpr (limits::max()
                          < std::numeric_limits<std::uint64_t>::max())
            {
                if (!(info.value <= limits::max()))
                {
                    return errc::item_value_out_of_range;
                }
            }
        }
        else
        {
            // ensure type == posint or negint
            if ((info.type & static_cast<std::uint8_t>(type_code::negint))
                != info.type)
            {
                return errc::item_type_mismatch;
            }
            // fused check for both positive and negative integers
            // negative integers are encoded as (-1 -n)
            // therefore the largest representable additional
            // information value is the same as the smallest one
            // e.g. a signed 8bit two's complement min() is -128 which
            // would be encoded as (-1 -[n=127])
            if (!(info.value <= limits::max()))
            {
                return errc::item_value_out_of_range;
            }
            std::uint64_t const signBit = static_cast<std::uint64_t>(info.type)
                                       << 58;
            std::int64_t const signExtended
                    = static_cast<std::int64_t>(signBit) >> 63;
            std::uint64_t const xorpad
                    = static_cast<std::uint64_t>(signExtended);

            info.value ^= xorpad;
        }
        dest = static_cast<T>(info.value);
        return success();
    }
};

template <iec559_floating_point T, input_stream Stream>
class basic_decoder<T, Stream>
{
    static_assert(std::numeric_limits<T>::is_iec559);

public:
    auto operator()(Stream &inStream, T &dest) const -> result<void>
    {
        DPLX_TRY(auto &&info, detail::parse_item_info(inStream));

        // #TODO maybe use specialized parsing logic for floats
        if (std::byte{info.type} != type_code::special
            || info.encoded_length < 3)
        {
            return errc::item_type_mismatch;
        }

        if (info.encoded_length == 9)
        {
            if constexpr (sizeof(T) == 8)
            {
                std::memcpy(&dest, &info.value, sizeof(T)); // #bit_cast
            }
            else
            {
                return errc::item_value_out_of_range;
            }
        }
        else if (info.encoded_length == 5)
        {
            if constexpr (sizeof(T) == 4)
            {
                std::memcpy(&dest, &info.value, sizeof(T)); // #bit_cast
            }
            else
            {
                static_assert(std::numeric_limits<float>::is_iec559);
                static_assert(sizeof(float) == 4);

                float tmp;
                std::memcpy(&tmp, &info.value, sizeof(tmp)); // #bit_cast
                dest = static_cast<T>(tmp);
            }
        }
        else if (info.encoded_length == 3)
        {
            // IEC 60559:2011 half precision
            // 1bit sign | 5bit exponent | 10bit significand
            // 0x8000    | 0x7C00        | 0x3ff

            // #TODO check whether I got the endianess right
            unsigned int significand = info.value & 0x3ff;
            int exponent = (info.value >> 10) & 0x1f;

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
            dest = static_cast<T>((info.value & 0x8000) == 0 ? value : -value);
        }
        return success();
    }
};

template <input_stream Stream>
class basic_decoder<bool, Stream>
{
public:
    auto operator()(Stream &stream, bool &dest) const -> result<void>
    {
        static constexpr auto boolPattern = to_byte(type_code::bool_false);

        DPLX_TRY(auto &&readProxy, dp::read(stream, 1));
        auto const memory = std::ranges::data(readProxy);
        auto const value = *memory;

        if constexpr (lazy_input_stream<Stream>)
        {
            DPLX_TRY(dp::consume(readProxy));
        }

        if ((value & boolPattern) != boolPattern)
        {
            return errc::item_type_mismatch;
        }

        dest = static_cast<bool>(
                static_cast<std::uint8_t>(value & std::byte{1}));
        return success();
    }
};

template <codable_enum Enum, input_stream Stream>
class basic_decoder<Enum, Stream>
{
    using underlying_type = std::underlying_type_t<Enum>;
    using underlying_decoder = basic_decoder<underlying_type, Stream>;

public:
    using value_type = Enum;

    auto operator()(Stream &stream, Enum &value) const -> result<void>
    {
        underlying_type bitRepresentation;
        DPLX_TRY(underlying_decoder()(stream, bitRepresentation));

        value = static_cast<Enum>(bitRepresentation);
        return oc::success();
    }
};

} // namespace dplx::dp
