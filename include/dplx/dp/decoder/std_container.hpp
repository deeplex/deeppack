
// Copyright Henrik Steffen Gaßmann 2020.
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <array>
#include <span>

#include <concepts>

#include <dplx/dp/concepts.hpp>
#include <dplx/dp/decoder/api.hpp>
#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/fwd.hpp>
#include <dplx/dp/item_parser.hpp>
#include <dplx/dp/stream.hpp>
#include <dplx/dp/type_code.hpp>

#include <boost/predef/other/workaround.h>

namespace dplx::dp
{

template <typename T>
inline constexpr bool disable_sequence_container = false;

// clang-format off
template <typename T>
concept sequence_container = !disable_sequence_container<T> &&
                             requires(T &&t, typename T::value_type &&v)
{
    t.push_back(v);
    { t.back() } -> std::same_as<typename T::reference>;
};
// clang-format on

template <typename T>
inline constexpr bool disable_associative_container = false;

// clang-format off
template <typename T>
concept associative_container = !disable_associative_container<T> &&
                                requires(T &&t, typename T::key_type &&k, typename T::mapped_type &&m)
{
    t.insert(typename T::value_type{k, m});
};
    // clang-format on

#if BOOST_PREDEF_WORKAROUND(BOOST_COMP_MSVC, <, 19, 29, 0)
namespace detail
{

template <typename T>
concept resizable_sequence_container = requires(T &&value)
{
    value.reserve(std::size_t{});
};

} // namespace detail
#endif

template <sequence_container T, input_stream Stream>
requires decodable<typename T::value_type, Stream> class basic_decoder<T,
                                                                       Stream>
{
    using element_type = typename T::value_type;
    using element_decoder = basic_decoder<element_type, Stream>;

public:
    auto operator()(Stream &stream, T &value) const -> result<void>
    {
        DPLX_TRY(auto &&arrayInfo, detail::parse_item_info(stream));

        if (static_cast<std::byte>(arrayInfo.type & 0b111'00000) !=
            type_code::array)
        {
            return errc::item_type_mismatch;
        }

        if (!arrayInfo.indefinite())
        {
            if (arrayInfo.value > std::numeric_limits<std::size_t>::max())
            {
                return errc::not_enough_memory;
            }
            DPLX_TRY(auto &&remainingInputSize,
                     dp::available_input_size(stream));
            if (remainingInputSize < arrayInfo.value)
            {
                return errc::missing_data;
            }
#if BOOST_PREDEF_WORKAROUND(BOOST_COMP_MSVC, <, 19, 29, 0)
            if constexpr (detail::resizable_sequence_container<T>)
#else
            if constexpr (requires { value.reserve(std::size_t{}); })
#endif
            {
                try
                {
                    value.reserve(static_cast<std::size_t>(arrayInfo.value));
                }
                catch (std::bad_alloc const &)
                {
                    return errc::not_enough_memory;
                }
            }

            for (std::size_t i = 0; i < arrayInfo.value; ++i)
            {
                value.push_back(element_type{});
                DPLX_TRY(element_decoder()(stream, value.back()));
            }

            return success();
        }
        else
        {
            for (;;)
            {
                {
                    DPLX_TRY(auto &&maybeStop, dp::read(stream, 1));
                    if (std::ranges::data(maybeStop)[0] ==
                        type_code::special_break)
                    {
                        if constexpr (lazy_input_stream<Stream>)
                        {
                            DPLX_TRY(dp::consume(stream, maybeStop));
                        }
                        break;
                    }

                    DPLX_TRY(dp::consume(stream, maybeStop, 0));
                }

                value.push_back(element_type{});
                DPLX_TRY(element_decoder()(stream, value.back()));
            }

            return success();
        }
    }
};

template <associative_container T, input_stream Stream>
requires decodable<typename T::key_type, Stream>
    &&decodable<typename T::mapped_type, Stream> class basic_decoder<T, Stream>
{
    using value_type = typename T::value_type;
    using key_type = typename T::key_type;
    using key_decoder = basic_decoder<key_type, Stream>;
    using mapped_type = typename T::mapped_type;
    using mapped_decoder = basic_decoder<mapped_type, Stream>;

public:
    auto operator()(Stream &stream, T &value) const -> result<void>
    {
        DPLX_TRY(auto &&mapInfo, detail::parse_item_info(stream));

        if (static_cast<std::byte>(mapInfo.type & 0b111'00000) !=
            type_code::map)
        {
            return errc::item_type_mismatch;
        }

        if (!mapInfo.indefinite())
        {
            if (mapInfo.value > std::numeric_limits<std::size_t>::max())
            {
                return errc::not_enough_memory;
            }
            DPLX_TRY(auto &&remainingInputSize,
                     dp::available_input_size(stream));
            if (remainingInputSize < mapInfo.value)
            {
                return errc::missing_data;
            }

            for (std::size_t i = 0; i < mapInfo.value; ++i)
            {
                key_type k{};
                DPLX_TRY(key_decoder()(stream, k));

                mapped_type m{};
                DPLX_TRY(mapped_decoder()(stream, m));

                value.insert(value_type{k, m});
            }

            return success();
        }
        else
        {
            for (;;)
            {
                {
                    DPLX_TRY(auto &&maybeStop, dp::read(stream, 1));
                    if (std::ranges::data(maybeStop)[0] ==
                        type_code::special_break)
                    {
                        if constexpr (lazy_input_stream<Stream>)
                        {
                            DPLX_TRY(dp::consume(stream, maybeStop));
                        }
                        break;
                    }

                    DPLX_TRY(dp::consume(stream, maybeStop, 0));
                }

                key_type k{};
                DPLX_TRY(key_decoder()(stream, k));

                mapped_type m{};
                DPLX_TRY(mapped_decoder()(stream, m));

                value.insert(value_type{k, m});
            }

            return success();
        }
    }
};

template <input_stream Stream>
class basic_decoder<std::span<std::byte>, Stream>
{
public:
    using value_type = std::span<std::byte>;

    inline auto operator()(Stream &inStream, value_type value) const
        -> result<void>
    {
        DPLX_TRY(auto &&headInfo, detail::parse_item_info(inStream));

        if (static_cast<std::byte>(headInfo.type & 0b111'00000) !=
            type_code::binary)
        {
            return errc::item_type_mismatch;
        }
        if (!headInfo.indefinite())
        {
            if (headInfo.value != value.size())
            {
                return errc::tuple_size_mismatch;
            }
            DPLX_TRY(dp::read(inStream, value.data(), value.size()));
        }
        else
        {
            auto writePointer = value.data();
            auto const endPointer = writePointer + value.size();

            while (writePointer != endPointer)
            {
                DPLX_TRY(auto &&subItemInfo, detail::parse_item_info(inStream));

                if (subItemInfo.type == 0b111'00001) // special break
                {
                    return errc::tuple_size_mismatch;
                }
                if (static_cast<std::byte>(subItemInfo.type) !=
                    type_code::binary)
                {
                    return errc::invalid_indefinite_subitem;
                }

                if (auto const remainingSpace =
                        static_cast<std::size_t>(endPointer - writePointer);
                    remainingSpace < subItemInfo.value)
                {
                    return errc::tuple_size_mismatch;
                }
                DPLX_TRY(dp::read(inStream,
                                  writePointer,
                                  static_cast<std::size_t>(subItemInfo.value)));

                writePointer += static_cast<std::size_t>(subItemInfo.value);
            }

            DPLX_TRY(auto &&stopCode, dp::read(inStream, 1));
            if (std::ranges::data(stopCode)[0] != type_code::special_break)
            {
                return errc::invalid_indefinite_subitem;
            }
        }
        return success();
    }
};

template <typename T, input_stream Stream>
requires decodable<T, Stream> class basic_decoder<std::span<T>, Stream>
{
public:
    using value_type = std::span<T>;

    inline auto operator()(Stream &inStream, value_type value) const
        -> result<void>
    {
        DPLX_TRY(auto &&headInfo, detail::parse_item_info(inStream));
        if (static_cast<std::byte>(headInfo.type & 0b111'00000) !=
            type_code::array)
        {
            return errc::item_type_mismatch;
        }

        if (headInfo.value != value.size() && !headInfo.indefinite())
        {
            return errc::tuple_size_mismatch;
        }
        for (auto &subItem : value)
        {
            DPLX_TRY(dp::decode(inStream, subItem));
        }
        if (headInfo.indefinite())
        {
            DPLX_TRY(auto &&stopCode, dp::read(inStream, 1));
            if (std::ranges::data(stopCode)[0] != type_code::special_break)
            {
                return errc::invalid_indefinite_subitem;
            }
        }
        return success();
    }
};

template <typename T, input_stream Stream>
requires detail::
    decodable_pair_like<T, Stream> class basic_decoder<std::span<T>, Stream>
{
public:
    using value_type = std::span<T>;

    inline auto operator()(Stream &inStream, value_type &value) const
        -> result<void>
    {
        DPLX_TRY(auto &&headInfo, detail::parse_item_info(inStream));
        if (static_cast<std::byte>(headInfo.type & 0b111'00000) !=
            type_code::map)
        {
            return errc::item_type_mismatch;
        }

        if (headInfo.value != value.size() && !headInfo.indefinite())
        {
            return errc::tuple_size_mismatch;
        }
        for (auto &subItem : value)
        {
            DPLX_TRY(dp::decode(inStream, std::get<0>(subItem)));
            DPLX_TRY(dp::decode(inStream, std::get<1>(subItem)));
        }
        if (headInfo.indefinite())
        {
            DPLX_TRY(auto &&stopCode, dp::read(inStream, 1));
            if (std::ranges::data(stopCode)[0] != type_code::special_break)
            {
                return errc::tuple_size_mismatch;
            }
        }
        return success();
    }
};

template <typename T, std::size_t N, input_stream Stream>
requires decodable<T, Stream> class basic_decoder<std::span<T, N>, Stream>
    : public basic_decoder<std::span<T>, Stream>
{
};

template <typename T, std::size_t N, input_stream Stream>
requires decodable<T, Stream> class basic_decoder<std::array<T, N>, Stream>
    : public basic_decoder<std::span<T>, Stream>
{
};

} // namespace dplx::dp
