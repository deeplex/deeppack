
// Copyright Henrik Steffen Gaßmann 2020.
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <array>
#include <concepts>
#include <ranges>
#include <span>

#include <dplx/dp/concepts.hpp>
#include <dplx/dp/decoder/api.hpp>
#include <dplx/dp/decoder/array_utils.hpp>
#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/fwd.hpp>
#include <dplx/dp/item_parser.hpp>
#include <dplx/dp/stream.hpp>
#include <dplx/dp/type_code.hpp>

// sequence container
namespace dplx::dp
{

template <typename T>
inline constexpr bool disable_sequence_container = false;

// clang-format off
template <typename X>
concept sequence_container
    = !disable_sequence_container<X> &&
      std::ranges::range<X> &&
      std::same_as<typename X::value_type, std::ranges::range_value_t<X>> &&
      requires(X c,
            std::ranges::sentinel_t<X> p)
        {
            typename X::value_type;
            { c.emplace(p) }
                -> std::same_as<std::ranges::iterator_t<X>>;
        };
// clang-format on

template <typename T>
inline constexpr bool disable_back_insertion_sequence_container = false;

// clang-format off
template <typename X>
concept back_insertion_sequence_container
    = !disable_back_insertion_sequence_container<X> &&
      sequence_container<X> &&
      requires(X c)
        {
            c.emplace_back();
            { c.back() } -> std::same_as<typename X::value_type &>;
        };
// clang-format on

template <sequence_container T, input_stream Stream>
    requires decodable<typename T::value_type, Stream>
class basic_decoder<T, Stream>
{
    using element_type = typename T::value_type;
    using element_decoder = basic_decoder<element_type, Stream>;

public:
    auto operator()(Stream &stream, T &value) const -> result<void>
    {
        return dp::parse_array(stream, value, type_code::array, decode_element);
    }

private:
    static auto decode_element(Stream &stream, T &value) -> result<void>
    {
        element_type *e;
        if constexpr (back_insertion_sequence_container<T>)
        {
            value.emplace_back();
            e = &value.back();
        }
        else
        {
            auto it = value.emplace(std::ranges::end(value));
            e = &(*it);
        }

        DPLX_TRY(element_decoder()(stream, *e));
        return oc::success();
    }
};

} // namespace dplx::dp

// associative containers
namespace dplx::dp
{

template <typename T>
inline constexpr bool disable_associative_container = false;

namespace detail
{

// clang-format off
template <typename T, typename Iterator>
concept associative_container_emplacement_result
    = pair_like<T> &&
        std::same_as<typename std::tuple_element<0, T>::type, Iterator> &&
        std::same_as<typename std::tuple_element<1, T>::type, bool>;
// clang-format on

} // namespace detail

// clang-format off
template <typename X>
concept associative_container
    = !disable_associative_container<X> &&
      std::ranges::range<X> &&
      std::same_as<typename X::value_type, std::ranges::range_value_t<X>> &&
      std::default_initializable<typename X::value_type> &&
      requires(X &&t, typename X::value_type v, std::ranges::iterator_t<X> hint)
        {
            typename X::value_type;
            { t.emplace(static_cast<typename X::value_type &&>(v)) }
                -> detail::associative_container_emplacement_result<
                        std::ranges::iterator_t<X>>;
        };
// clang-format on

// clang-format off
template <typename X>
concept map_like_associative_container
    = associative_container<X> &&
      pair_like<typename X::value_type> &&
      std::default_initializable<typename X::key_type> &&
      std::default_initializable<typename X::mapped_type> &&
      requires(X &&t, typename X::key_type &&k, typename X::mapped_type &&m, std::ranges::iterator_t<X> hint)
        {
            typename X::key_type;
            typename X::mapped_type;
            { t.emplace(static_cast<typename X::key_type &&>(k),
                        static_cast<typename X::mapped_type &&>(m)) }
                -> detail::associative_container_emplacement_result<
                        std::ranges::iterator_t<X>>;
        };
// clang-format on

template <associative_container T, input_stream Stream>
    requires decodable<typename T::value_type, Stream>
class basic_decoder<T, Stream>
{
    using element_type = typename T::value_type;
    using element_decoder = basic_decoder<element_type, Stream>;

public:
    auto operator()(Stream &stream, T &value) const -> result<void>
    {
        return dp::parse_array(stream, value, type_code::array, decode_element);
    }

private:
    static auto decode_element(Stream &stream, T &value) -> result<void>
    {
        element_type e{};
        DPLX_TRY(element_decoder()(stream, e));

        if (auto &&[it, inserted]
            = value.emplace(static_cast<element_type &&>(e));
            !inserted)
        {
            return errc::duplicate_key;
        }
        return oc::success();
    }
};

template <map_like_associative_container T, input_stream Stream>
    requires(decodable<typename T::key_type, Stream>
                     &&decodable<typename T::mapped_type, Stream>)
class basic_decoder<T, Stream>
{
    using key_type = typename T::key_type;
    using key_decoder = basic_decoder<key_type, Stream>;
    using mapped_type = typename T::mapped_type;
    using mapped_decoder = basic_decoder<mapped_type, Stream>;

public:
    auto operator()(Stream &stream, T &value) const -> result<void>
    {
        return dp::parse_array(stream, value, type_code::map, decode_element);
    }

private:
    static auto decode_element(Stream &stream, T &value) -> result<void>
    {
        key_type k{};
        DPLX_TRY(key_decoder()(stream, k));

        mapped_type m{};
        DPLX_TRY(mapped_decoder()(stream, m));

        if (auto &&[it, inserted]
            = value.emplace(static_cast<key_type &&>(k),
                            static_cast<mapped_type &&>(m));
            !inserted)
        {
            return errc::duplicate_key;
        }
        return oc::success();
    }
};

} // namespace dplx::dp

// span<std::byte> & span<T>
namespace dplx::dp
{

template <input_stream Stream>
class basic_decoder<std::span<std::byte>, Stream>
{
public:
    using value_type = std::span<std::byte>;

    inline auto operator()(Stream &inStream, value_type value) const
            -> result<void>
    {
        DPLX_TRY(auto &&headInfo, detail::parse_item_info(inStream));

        if (static_cast<std::byte>(headInfo.type & 0b111'00000)
            != type_code::binary)
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
                if (static_cast<std::byte>(subItemInfo.type)
                    != type_code::binary)
                {
                    return errc::invalid_indefinite_subitem;
                }

                if (auto const remainingSpace
                    = static_cast<std::size_t>(endPointer - writePointer);
                    remainingSpace < subItemInfo.value)
                {
                    return errc::tuple_size_mismatch;
                }
                DPLX_TRY(dp::read(inStream, writePointer,
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
    requires decodable<T, Stream>
class basic_decoder<std::span<T>, Stream>
{
public:
    using value_type = std::span<T>;

    inline auto operator()(Stream &inStream, value_type value) const
            -> result<void>
    {
        DPLX_TRY(auto &&headInfo, detail::parse_item_info(inStream));
        if (static_cast<std::byte>(headInfo.type & 0b111'00000)
            != type_code::array)
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
    requires detail::decodable_pair_like<T, Stream>
class basic_decoder<std::span<T>, Stream>
{
public:
    using value_type = std::span<T>;

    inline auto operator()(Stream &inStream, value_type &value) const
            -> result<void>
    {
        DPLX_TRY(auto &&headInfo, detail::parse_item_info(inStream));
        if (static_cast<std::byte>(headInfo.type & 0b111'00000)
            != type_code::map)
        {
            return errc::item_type_mismatch;
        }

        if (headInfo.value != value.size() && !headInfo.indefinite())
        {
            return errc::tuple_size_mismatch;
        }
        for (auto &subItem : value)
        {
            DPLX_TRY(dp::decode(inStream, get<0>(subItem)));
            DPLX_TRY(dp::decode(inStream, get<1>(subItem)));
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
    requires(decodable<
                     T,
                     Stream> || std::same_as<std::remove_const_t<T>, std::byte>)
class basic_decoder<std::span<T, N>, Stream>
    : public basic_decoder<std::span<T>, Stream>
{
};

template <typename T, std::size_t N, input_stream Stream>
    requires(decodable<
                     T,
                     Stream> || std::same_as<std::remove_const_t<T>, std::byte>)
class basic_decoder<std::array<T, N>, Stream>
    : public basic_decoder<std::span<T>, Stream>
{
};

} // namespace dplx::dp
