
// Copyright Henrik Steffen Gaßmann 2020-2021.
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>
#include <cstdint>

#include <ranges>
#include <type_traits>

#include <boost/container/small_vector.hpp>
#include <boost/predef/compiler.h>
#include <boost/predef/other/workaround.h>

#include <dplx/dp/detail/bit.hpp>
#include <dplx/dp/detail/item_size.hpp>
#include <dplx/dp/detail/parse_item.hpp>
#include <dplx/dp/detail/utils.hpp>
#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/stream.hpp>
#include <dplx/dp/type_code.hpp>

namespace dplx::dp
{

enum class parse_mode
{
    lenient,
    canonical,
    strict,
};

template <typename Container>
concept string_output_container = container_traits<
        Container>::resize && std::ranges::contiguous_range<Container>;

// clang-format off
template <typename Fn, typename InputStream, typename Container>
concept subitem_parslet = input_stream<InputStream>
    && ((
            std::invocable<Fn, InputStream &, Container &, std::size_t const>
                        && detail::tryable<std::invoke_result_t<Fn, InputStream &, Container &, std::size_t const>>)
            
        || (
            std::invocable<Fn, InputStream &, Container &, std::size_t const, parse_mode const>
                        && detail::tryable<std::invoke_result_t<Fn, InputStream &, Container &, std::size_t const, parse_mode const>>)
            );
// clang-format on

template <input_stream Stream>
class item_parser
{
    using straits = stream_traits<Stream>;
    using parse = item_parser;
    static constexpr auto size_t_max = std::numeric_limits<std::size_t>::max();

public:
    static inline auto generic(Stream &inStream) -> result<item_info>
    {
        return detail::parse_item(inStream);
    }

    static inline auto expect(Stream &inStream,
                              type_code const type,
                              std::uint64_t const value,
                              parse_mode const mode = parse_mode::lenient)
            -> result<void>
    {
        DPLX_TRY(item_info const item, parse::generic(inStream));

        if (item.type != type)
            DPLX_ATTR_UNLIKELY
            {
                return errc::item_type_mismatch;
            }
        if (item.indefinite())
            DPLX_ATTR_UNLIKELY
            {
                if (type == type_code::special)
                {
                    if (value == 0x1fu)
                    {
                        return oc::success();
                    }
                    // unwanted special break.
                    return errc::item_type_mismatch;
                }
                return errc::indefinite_item;
            }
        if (mode != parse_mode::lenient
            && detail::var_uint_encoded_size(item.value)
                       < static_cast<unsigned>(item.encoded_length))
            DPLX_ATTR_UNLIKELY
            {
                return errc::oversized_additional_information_coding;
            }
        if (item.value != value)
            DPLX_ATTR_UNLIKELY
            {
                return errc::item_value_out_of_range;
            }
        return oc::success();
    }

    template <integer T>
    static inline auto integer(Stream &inStream,
                               parse_mode const mode = parse_mode::lenient)
            -> result<T>
    {
        DPLX_TRY(item_info const item, parse::generic(inStream));

        if constexpr (std::is_unsigned_v<T>)
        {
            if (item.type != type_code::posint)
            {
                return errc::item_type_mismatch;
            }
        }
        else
        {
            if (item.type != type_code::posint
                && item.type != type_code::negint)
            {
                return errc::item_type_mismatch;
            }
        }

        // fused check for both positive and negative integers
        // negative integers are encoded as (-1 -n)
        // therefore the largest representable additional
        // information value is the same as the smallest one
        // e.g. a signed 8bit two's complement min() is -128 which
        // would be encoded as (-1 -[n=127])
        if (item.value > static_cast<std::make_unsigned_t<T>>(
                    std::numeric_limits<T>::max()))
        {
            return errc::item_value_out_of_range;
        }
        if (mode != parse_mode::lenient
            && detail::var_uint_encoded_size(item.value)
                       < static_cast<unsigned>(item.encoded_length))
        {
            return errc::oversized_additional_information_coding;
        }

        if constexpr (std::is_unsigned_v<T>)
        {
            return static_cast<T>(item.value);
        }
        else
        {
            std::uint64_t const signBit = static_cast<std::uint64_t>(item.type)
                                       << 58;
            std::int64_t const signExtended
                    = static_cast<std::int64_t>(signBit) >> 63;
            std::uint64_t const xorpad
                    = static_cast<std::uint64_t>(signExtended);

            return static_cast<T>(item.value ^ xorpad);
        }
    }

    template <string_output_container StringType>
    static inline auto binary(Stream &inStream,
                              StringType &dest,
                              parse_mode const mode = parse_mode::lenient)
            -> result<std::size_t>
    {
        return parse::string(inStream, dest, size_t_max, mode,
                             type_code::binary);
    }
    template <string_output_container StringType>
    static inline auto binary(Stream &inStream,
                              StringType &dest,
                              std::size_t const maxSize,
                              parse_mode const mode = parse_mode::lenient)
            -> result<std::size_t>
    {
        return parse::string(inStream, dest, maxSize, mode, type_code::binary);
    }
    template <string_output_container StringType>
    static inline auto binary_finite(Stream &inStream,
                                     StringType &dest,
                                     parse_mode const mode
                                     = parse_mode::lenient)
            -> result<std::size_t>
    {
        return parse::string_finite(inStream, dest, size_t_max, mode,
                                    type_code::binary);
    }
    template <string_output_container StringType>
    static inline auto binary_finite(Stream &inStream,
                                     StringType &dest,
                                     std::size_t const maxSize,
                                     parse_mode const mode
                                     = parse_mode::lenient)
            -> result<std::size_t>
    {
        return parse::string_finite(inStream, dest, maxSize, mode,
                                    type_code::binary);
    }

    template <typename T>
    static inline auto u8string(Stream &inStream,
                                T &dest,
                                parse_mode const mode = parse_mode::lenient)
            -> result<std::size_t>
    {
        return parse::string<T>(inStream, dest, size_t_max, mode,
                                type_code::text);
    }
    template <typename T>
    static inline auto u8string(Stream &inStream,
                                T &dest,
                                std::size_t const maxSize,
                                parse_mode const mode = parse_mode::lenient)
            -> result<std::size_t>
    {
        return parse::string<T>(inStream, dest, maxSize, mode, type_code::text);
    }
    template <typename T>
    static inline auto u8string_finite(Stream &inStream,
                                       T &dest,
                                       parse_mode const mode
                                       = parse_mode::lenient)
            -> result<std::size_t>
    {
        return parse::string_finite<T>(inStream, dest, size_t_max, mode,
                                       type_code::text);
    }
    template <typename T>
    static inline auto u8string_finite(Stream &inStream,
                                       T &dest,
                                       std::size_t const maxSize,
                                       parse_mode const mode
                                       = parse_mode::lenient)
            -> result<std::size_t>
    {
        return parse::string_finite<T>(inStream, dest, maxSize, mode,
                                       type_code::text);
    }

    template <typename Container, typename DecodeElementFn>
        requires subitem_parslet<DecodeElementFn, Stream, Container>
    static inline auto array(Stream &inStream,
                             Container &dest,
                             DecodeElementFn &&decodeElementFn,
                             parse_mode const mode = parse_mode::lenient)
            -> result<std::size_t>
    {
        return parse::array_like<Container, DecodeElementFn>(
                inStream, dest, size_t_max, mode, type_code::array,
                static_cast<DecodeElementFn &&>(decodeElementFn));
    }
    template <typename Container, typename DecodeElementFn>
        requires subitem_parslet<DecodeElementFn, Stream, Container>
    static inline auto array(Stream &inStream,
                             Container &dest,
                             std::size_t const maxSize,
                             DecodeElementFn &&decodeElementFn,
                             parse_mode const mode = parse_mode::lenient)
            -> result<std::size_t>
    {
        return parse::array_like<Container, DecodeElementFn>(
                inStream, dest, maxSize, mode, type_code::array,
                static_cast<DecodeElementFn &&>(decodeElementFn));
    }
    template <typename Container, typename DecodeElementFn>
        requires subitem_parslet<DecodeElementFn, Stream, Container>
    static inline auto array_finite(Stream &inStream,
                                    Container &dest,
                                    DecodeElementFn &&decodeElementFn,
                                    parse_mode const mode = parse_mode::lenient)
            -> result<std::size_t>
    {
        return parse::array_finite_like<Container, DecodeElementFn>(
                inStream, dest, size_t_max, mode, type_code::array,
                static_cast<DecodeElementFn &&>(decodeElementFn));
    }
    template <typename Container, typename DecodeElementFn>
        requires subitem_parslet<DecodeElementFn, Stream, Container>
    static inline auto array_finite(Stream &inStream,
                                    Container &dest,
                                    std::size_t const maxSize,
                                    DecodeElementFn &&decodeElementFn,
                                    parse_mode const mode = parse_mode::lenient)
            -> result<std::size_t>
    {
        return parse::array_finite_like<Container, DecodeElementFn>(
                inStream, dest, maxSize, mode, type_code::array,
                static_cast<DecodeElementFn &&>(decodeElementFn));
    }

    template <typename Container, typename DecodeElementFn>
        requires subitem_parslet<DecodeElementFn, Stream, Container>
    static inline auto map(Stream &inStream,
                           Container &dest,
                           DecodeElementFn &&decodeElementFn,
                           parse_mode const mode = parse_mode::lenient)
            -> result<std::size_t>
    {
        return parse::array_like<Container, DecodeElementFn>(
                inStream, dest, size_t_max, mode, type_code::map,
                static_cast<DecodeElementFn &&>(decodeElementFn));
    }
    template <typename Container, typename DecodeElementFn>
        requires subitem_parslet<DecodeElementFn, Stream, Container>
    static inline auto map(Stream &inStream,
                           Container &dest,
                           std::size_t const maxSize,
                           DecodeElementFn &&decodeElementFn,
                           parse_mode const mode = parse_mode::lenient)
            -> result<std::size_t>
    {
        return parse::array_like<Container, DecodeElementFn>(
                inStream, dest, maxSize, mode, type_code::map,
                static_cast<DecodeElementFn &&>(decodeElementFn));
    }
    template <typename Container, typename DecodeElementFn>
        requires subitem_parslet<DecodeElementFn, Stream, Container>
    static inline auto map_finite(Stream &inStream,
                                  Container &dest,
                                  DecodeElementFn &&decodeElementFn,
                                  parse_mode const mode = parse_mode::lenient)
            -> result<std::size_t>
    {
        return parse::array_finite_like<Container, DecodeElementFn>(
                inStream, dest, size_t_max, mode, type_code::map,
                static_cast<DecodeElementFn &&>(decodeElementFn));
    }
    template <typename Container, typename DecodeElementFn>
        requires subitem_parslet<DecodeElementFn, Stream, Container>
    static inline auto map_finite(Stream &inStream,
                                  Container &dest,
                                  std::size_t const maxSize,
                                  DecodeElementFn &&decodeElementFn,
                                  parse_mode const mode = parse_mode::lenient)
            -> result<std::size_t>
    {
        return parse::array_finite_like<Container, DecodeElementFn>(
                inStream, dest, maxSize, mode, type_code::map,
                static_cast<DecodeElementFn &&>(decodeElementFn));
    }

    static inline auto boolean(Stream &inStream) -> result<bool>
    {
        constexpr auto boolPattern
                = static_cast<std::uint8_t>(type_code::bool_false);

        DPLX_TRY(auto &&readProxy, read(inStream, 1));
        auto const memory = std::ranges::data(readProxy);
        auto const value = static_cast<std::uint8_t>(*memory);
        unsigned const rolled = value - boolPattern;

        if (rolled > 1u)
            DPLX_ATTR_UNLIKELY
            {
                DPLX_TRY(consume(readProxy, 0));
                return errc::item_type_mismatch;
            }

        if constexpr (lazy_input_stream<Stream>)
        {
            DPLX_TRY(consume(readProxy));
        }

        return static_cast<bool>(rolled);
    }

    static inline auto float_single(Stream &inStream) -> result<float>
    {
        DPLX_TRY(item_info const item, parse::generic(inStream));

        if (item.type != type_code::special || item.indefinite()
            || item.encoded_length < 3)
        {
            return errc::item_type_mismatch;
        }

        if (item.encoded_length == 9)
        {
            return errc::item_value_out_of_range;
        }
        else if (item.encoded_length == 5)
        {
            float value;
            std::memcpy(&value, &item.value, sizeof(value)); // #bit_cast
            return value;
        }
        else // if (item.encoded_length == 3)
        {
            return static_cast<float>(detail::load_iec559_half(
                    static_cast<std::uint16_t>(item.value)));
        }
    }
    static inline auto float_double(Stream &inStream) -> result<double>
    {
        DPLX_TRY(item_info const item, parse::generic(inStream));

        if (item.type != type_code::special || item.indefinite()
            || item.encoded_length < 3)
        {
            return errc::item_type_mismatch;
        }

        if (item.encoded_length == 9)
        {
            double value;
            std::memcpy(&value, &item.value, sizeof(value)); // #bit_cast
            return value;
        }
        else if (item.encoded_length == 5)
        {
            float value;
            std::memcpy(&value, &item.value, sizeof(value)); // #bit_cast
            return value;
        }
        else // if (item.encoded_length == 3)
        {
            return detail::load_iec559_half(
                    static_cast<std::uint16_t>(item.value));
        }
    }

private:
    template <typename StringType>
    static inline auto string(Stream &inStream,
                              StringType &dest,
                              std::size_t const maxSize,
                              parse_mode const mode,
                              type_code const expectedType)
            -> result<std::size_t>;
    template <typename StringType>
    static inline auto string_finite(Stream &inStream,
                                     StringType &dest,
                                     std::size_t const maxSize,
                                     parse_mode const mode,
                                     type_code const expectedType)
            -> result<std::size_t>;

    template <typename T, typename DecodeElementFn>
    static inline auto array_like(Stream &inStream,
                                  T &dest,
                                  std::size_t const maxSize,
                                  parse_mode const mode,
                                  type_code const expectedType,
                                  DecodeElementFn &&decodeElement)
            -> result<std::size_t>;
    template <typename T, typename DecodeElementFn>
    static inline auto array_finite_like(Stream &inStream,
                                         T &dest,
                                         std::size_t const maxSize,
                                         parse_mode const mode,
                                         type_code const expectedType,
                                         DecodeElementFn &&decodeElement)
            -> result<std::size_t>;
};

template <input_stream Stream>
template <typename StringType>
inline auto item_parser<Stream>::string(Stream &inStream,
                                        StringType &dest,
                                        std::size_t const maxSize,
                                        parse_mode const mode,
                                        type_code const expectedType)
        -> result<std::size_t>
{
    DPLX_TRY(item_info item, parse::generic(inStream));

    if (item.type != expectedType)
    {
        return errc::item_type_mismatch;
    }

    std::size_t size;
    if (!item.indefinite())
        DPLX_ATTR_LIKELY
        {
            if (mode != parse_mode::lenient
                && detail::var_uint_encoded_size(item.value)
                           < static_cast<unsigned>(item.encoded_length))
            {
                return errc::oversized_additional_information_coding;
            }

            DPLX_TRY(auto &&availableBytes, available_input_size(inStream));
            if (availableBytes < item.value)
            {
                return errc::missing_data;
            }
            if (item.value > maxSize)
            {
                return errc::string_exceeds_size_limit;
            }

            size = static_cast<std::size_t>(item.value);
            DPLX_TRY(container_resize_for_overwrite(dest, size));

            auto const memory = std::ranges::data(dest);

            DPLX_TRY(read(inStream, reinterpret_cast<std::byte *>(memory),
                          size));
        }
    else if (mode != parse_mode::lenient)
    {
        return errc::indefinite_item;
    }
    else
    {
        size = 0u;
        DPLX_TRY(container_resize(dest, std::size_t{}));

        for (;;)
        {
            DPLX_TRY(item_info chunkItem, parse::generic(inStream));

            if (chunkItem.type == type_code::special && chunkItem.indefinite())
            {
                // special break
                break;
            }
            else if (chunkItem.type != expectedType)
            {
                return errc::invalid_indefinite_subitem;
            }

            DPLX_TRY(auto &&availableBytes, available_input_size(inStream));
            if (availableBytes < chunkItem.value)
            {
                return errc::missing_data;
            }

            auto const newSize = size + chunkItem.value;
            if (newSize > maxSize)
            {
                return errc::string_exceeds_size_limit;
            }
            auto const chunkSize = static_cast<std::size_t>(chunkItem.value);

            DPLX_TRY(container_resize_for_overwrite(
                    dest, static_cast<std::size_t>(newSize)));

            auto const memory = std::ranges::data(dest);

            DPLX_TRY(read(inStream,
                          reinterpret_cast<std::byte *>(memory) + size,
                          chunkSize));

            size = static_cast<std::size_t>(newSize);
        }
    }

    // if (mode == parse_mode::strict && expectedType == type_code::text)
    //{
    //    // TODO: verify UTF-8 content
    //}

    return size;
}

template <input_stream Stream>
template <typename StringType>
inline auto item_parser<Stream>::string_finite(Stream &inStream,
                                               StringType &dest,
                                               std::size_t const maxSize,
                                               parse_mode const mode,
                                               type_code const expectedType)
        -> result<std::size_t>
{
    DPLX_TRY(item_info item, parse::generic(inStream));

    if (item.type != expectedType)
    {
        return errc::item_type_mismatch;
    }
    if (item.indefinite())
    {
        return errc::indefinite_item;
    }
    if (mode != parse_mode::lenient
        && detail::var_uint_encoded_size(item.value)
                   < static_cast<unsigned>(item.encoded_length))
    {
        return errc::oversized_additional_information_coding;
    }

    DPLX_TRY(auto const availableBytes, available_input_size(inStream));
    if (availableBytes < item.value)
    {
        return errc::missing_data;
    }
    if (item.value > maxSize)
    {
        return errc::string_exceeds_size_limit;
    }

    auto const byteSize = static_cast<std::size_t>(item.value);
    DPLX_TRY(container_resize_for_overwrite(dest, byteSize));

    auto const memory = std::ranges::data(dest);

    DPLX_TRY(read(inStream, reinterpret_cast<std::byte *>(memory), byteSize));

    // if (mode == parse_mode::strict && expectedType == type_code::text)
    //{
    //    // TODO: verify UTF-8 content
    //}

    return byteSize;
}

template <input_stream Stream>
template <typename T, typename DecodeElementFn>
inline auto item_parser<Stream>::array_like(Stream &inStream,
                                            T &dest,
                                            std::size_t const maxSize,
                                            parse_mode const mode,
                                            type_code const expectedType,
                                            DecodeElementFn &&decodeElement)
        -> result<std::size_t>
{
    DPLX_TRY(item_info item, parse::generic(inStream));

    if (item.type != expectedType)
    {
        return errc::item_type_mismatch;
    }

    if (!item.indefinite())
    {
        DPLX_TRY(unsigned_integer auto remainingInputSize,
                 available_input_size(inStream));

        remainingInputSize >>= (expectedType == type_code::map ? 1 : 0);
        if (remainingInputSize < item.value)
        {
            return errc::missing_data;
        }
        if (item.value > maxSize)
        {
            return errc::item_value_out_of_range;
        }

        auto const numElements = static_cast<std::size_t>(item.value);
        if constexpr (nothrow_tag_invocable<container_reserve_fn, T &,
                                            std::size_t const>)
        {
            DPLX_TRY(container_reserve(dest, numElements));
        }

        for (std::size_t i = 0; i < numElements; ++i)
        {
            std::size_t const v = i;
#if BOOST_PREDEF_WORKAROUND(BOOST_COMP_MSVC, <, 19, 30, 0)
            if constexpr (std::invocable<DecodeElementFn, Stream &, T &,
                                         std::size_t const, parse_mode const>)
#else
            if constexpr (requires { decodeElement(inStream, dest, v, mode); })
#endif
            {
                DPLX_TRY(decodeElement(inStream, dest, v, mode));
            }
            else
            {
                DPLX_TRY(decodeElement(inStream, dest, v));
            }
        }
        return numElements;
    }
    else if (mode != parse_mode::lenient)
    {
        return errc::indefinite_item;
    }
    else
    {
        std::size_t i = 0;
        for (;; ++i)
        {
            if (i > maxSize)
                DPLX_ATTR_UNLIKELY
                {
                    return errc::item_value_out_of_range;
                }

            {
                DPLX_TRY(auto &&maybeStop, read(inStream, 1));
                if (std::ranges::data(maybeStop)[0] == type_code::special_break)
                {
                    if constexpr (lazy_input_stream<Stream>)
                    {
                        DPLX_TRY(consume(inStream, maybeStop));
                    }
                    break;
                }

                DPLX_TRY(consume(inStream, maybeStop, 0));
            }

            std::size_t const v = i;
#if BOOST_PREDEF_WORKAROUND(BOOST_COMP_MSVC, <, 19, 30, 0)
            if constexpr (std::invocable<DecodeElementFn, Stream &, T &,
                                         std::size_t const, parse_mode const>)
#else
            if constexpr (requires { decodeElement(inStream, dest, v, mode); })
#endif
            {
                DPLX_TRY(decodeElement(inStream, dest, v, mode));
            }
            else
            {
                DPLX_TRY(decodeElement(inStream, dest, v));
            }
        }
        return i;
    }
}

template <input_stream Stream>
template <typename T, typename DecodeElementFn>
inline auto
item_parser<Stream>::array_finite_like(Stream &inStream,
                                       T &dest,
                                       std::size_t const maxSize,
                                       parse_mode const mode,
                                       type_code const expectedType,
                                       DecodeElementFn &&decodeElement)
        -> result<std::size_t>
{
    DPLX_TRY(item_info item, parse::generic(inStream));

    if (item.type != expectedType)
    {
        return errc::item_type_mismatch;
    }
    if (item.indefinite())
    {
        return errc::indefinite_item;
    }
    if (mode != parse_mode::lenient
        && detail::var_uint_encoded_size(item.value)
                   < static_cast<unsigned>(item.encoded_length))
    {
        return errc::oversized_additional_information_coding;
    }

    DPLX_TRY(unsigned_integer auto remainingInputSize,
             available_input_size(inStream));

    remainingInputSize >>= (expectedType == type_code::map ? 1 : 0);
    if (remainingInputSize < item.value)
    {
        return errc::missing_data;
    }
    if (item.value > maxSize)
    {
        return errc::item_value_out_of_range;
    }

    auto const numElements = static_cast<std::size_t>(item.value);
    DPLX_TRY(container_reserve(dest, numElements));

    for (std::size_t i = 0; i < numElements; ++i)
    {
        std::size_t const v = i;
#if BOOST_PREDEF_WORKAROUND(BOOST_COMP_MSVC, <, 19, 30, 0)
        if constexpr (std::invocable<DecodeElementFn, Stream &, T &,
                                     std::size_t const, parse_mode const>)
#else
        if constexpr (requires { decodeElement(inStream, dest, v, mode); })
#endif
        {
            DPLX_TRY(decodeElement(inStream, dest, v, mode));
        }
        else
        {
            DPLX_TRY(decodeElement(inStream, dest, v));
        }
    }
    return numElements;
}

} // namespace dplx::dp
