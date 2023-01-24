
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

#include <dplx/dp/codecs/std-container.hpp>
#include <dplx/dp/concepts.hpp>
#include <dplx/dp/customization.std.hpp>
#include <dplx/dp/decoder/api.hpp>
#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/fwd.hpp>
#include <dplx/dp/item_parser.hpp>
#include <dplx/dp/items/type_code.hpp>
#include <dplx/dp/stream.hpp>

// sequence container
namespace dplx::dp
{

template <sequence_container T, input_stream Stream>
    requires decodable<typename T::value_type, Stream>
class basic_decoder<T, Stream>
{
    using parse = item_parser<Stream>;
    using element_type = typename T::value_type;
    using element_decoder = basic_decoder<element_type, Stream>;

public:
    auto operator()(Stream &stream, T &value) const -> result<void>
    {
        DPLX_TRY(parse::array(stream, value, decode_element));
        return oc::success();
    }

private:
    static auto decode_element(Stream &stream, T &value, std::size_t const)
            -> result<void>
    {
        element_type *e = &value.emplace_back();

        DPLX_TRY(element_decoder()(stream, *e));
        return oc::success();
    }
};

} // namespace dplx::dp

// associative containers
namespace dplx::dp
{

template <associative_container T, input_stream Stream>
    requires decodable<typename T::value_type, Stream>
class basic_decoder<T, Stream>
{
    using parse = item_parser<Stream>;
    using element_type = typename T::value_type;
    using element_decoder = basic_decoder<element_type, Stream>;

public:
    auto operator()(Stream &stream, T &value) const -> result<void>
    {
        return parse::array(stream, value, decode_element);
    }

private:
    static auto decode_element(Stream &stream, T &value, std::size_t const)
            -> result<void>
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

template <mapping_associative_container T, input_stream Stream>
    requires(decodable<typename T::key_type, Stream>
                     &&decodable<typename T::mapped_type, Stream>)
class basic_decoder<T, Stream>
{
    using parse = item_parser<Stream>;
    using key_type = typename T::key_type;
    using key_decoder = basic_decoder<key_type, Stream>;
    using mapped_type = typename T::mapped_type;
    using mapped_decoder = basic_decoder<mapped_type, Stream>;

public:
    auto operator()(Stream &stream, T &value) const -> result<void>
    {
        return parse::map(stream, value, decode_element);
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

// std::array
namespace dplx::dp
{

namespace detail
{

template <typename T, input_stream Stream>
class basic_array_decoder
{
    using parse = item_parser<Stream>;

public:
    inline auto operator()(Stream &inStream, std::span<T> value) const
            -> result<void>
    {
        DPLX_TRY(auto numElements,
                 parse::array(inStream, value, decode_element));
        if (numElements != value.size())
        {
            return errc::tuple_size_mismatch;
        }
        return oc::success();
    }

private:
    static inline auto decode_element(Stream &inStream,
                                      std::span<T> &storage,
                                      std::size_t const i) -> result<void>
    {
        return dp::decode(inStream, storage[i]);
    }
};

template <input_stream Stream>
class basic_array_decoder<std::byte, Stream>
{
    using parse = item_parser<Stream>;

public:
    inline auto operator()(Stream &inStream, std::span<std::byte> value) const
            -> result<void>
    {
        DPLX_TRY(auto const size, parse::binary_finite(inStream, value));
        if (size != value.size())
        {
            return errc::tuple_size_mismatch;
        }
        return oc::success();
    }
};

} // namespace detail

template <typename T, std::size_t N, input_stream Stream>
    requires(decodable<T, Stream> || std::same_as<T, std::byte>)
class basic_decoder<std::array<T, N>, Stream>
    : public detail::basic_array_decoder<T, Stream>
{
public:
    using value_type = std::array<T, N>;
};

} // namespace dplx::dp
