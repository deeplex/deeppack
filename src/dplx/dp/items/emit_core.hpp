
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include <dplx/cncr/math_supplement.hpp>

#include <dplx/dp/detail/bit.hpp>
#include <dplx/dp/detail/utils.hpp>
#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/items/emit_context.hpp>
#include <dplx/dp/streams/output_buffer.hpp>
#include <dplx/dp/type_code.hpp>

namespace dplx::dp::detail
{

inline auto store_var_uint_eos(output_buffer &out,
                               std::uint64_t value,
                               unsigned category,
                               unsigned bytePowerPlus2,
                               unsigned byteSize) noexcept -> result<void>
{
    if (out.size() < byteSize) [[unlikely]]
    {
        DPLX_TRY(out.ensure_size(byteSize));
    }

    auto *const dest = out.data();
    // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    switch (bytePowerPlus2)
    {
    case 1U:
        dest[0] = static_cast<std::byte>(value);
        break;
    case 2U:
        dest[0] = static_cast<std::byte>(category | 24U);
        dest[1] = static_cast<std::byte>(value);
        break;
    case 3U:
        dest[0] = static_cast<std::byte>(category | 25U);
        detail::store(dest + 1, static_cast<std::uint16_t>(value));
        break;
    case 4U:
        dest[0] = static_cast<std::byte>(category | 26U);
        detail::store(dest + 1, static_cast<std::uint32_t>(value));
        break;
    case 5U:
        dest[0] = static_cast<std::byte>(category | 27U);
        detail::store(dest + 1, value);
        break;
    }
    // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    // NOLINTEND(cppcoreguidelines-avoid-magic-numbers)

    out.commit_written(byteSize);
    return oc::success();
}

template <typename T>
    requires(!std::is_signed_v<T>)
inline auto store_var_uint(output_buffer &out,
                           T value,
                           type_code const category) noexcept -> result<void>
{
    auto rcategory = static_cast<unsigned>(category);
    // NOLINTBEGIN(cppcoreguidelines-init-variables)
    unsigned bytePowerPlus2;
    unsigned bitSize;
    unsigned byteSize;
    // NOLINTEND(cppcoreguidelines-init-variables)
    if (value <= detail::inline_value_max)
    {
        bytePowerPlus2 = 1U;
        bitSize = 1U;
        byteSize = 1U;
        value += rcategory;
        rcategory = static_cast<unsigned>(value - detail::inline_value_max);
    }
    else
    {
        auto const lastSetBitIndex
                = static_cast<unsigned>(detail::find_last_set_bit(value));
        bytePowerPlus2 = static_cast<unsigned>(
                detail::find_last_set_bit(lastSetBitIndex));

        bitSize = 2U << bytePowerPlus2;
        byteSize = 1U + (bitSize >> 3);
    }

    if (out.size() < 1U + sizeof(T)) [[unlikely]]
    {
        return store_var_uint_eos(out, static_cast<std::uint64_t>(value),
                                  rcategory, bytePowerPlus2, byteSize);
    }

    auto *const dest = out.data();
    *dest = static_cast<std::byte>(rcategory + detail::inline_value_max
                                   + bytePowerPlus2 - 1);

    T const encoded = value << (detail::digits_v<T> - bitSize);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    detail::store(dest + 1, encoded);

    out.commit_written(byteSize);
    return oc::success();
}

inline auto store_inline_value(output_buffer &out,
                               unsigned const value,
                               type_code const category) noexcept
        -> result<void>
{
    constexpr std::size_t encodedSize = 1U;
    if (out.empty()) [[unlikely]]
    {
        DPLX_TRY(out.ensure_size(encodedSize));
    }

    *out.data()
            = static_cast<std::byte>(value | static_cast<unsigned>(category));

    out.commit_written(encodedSize);
    return oc::success();
}

} // namespace dplx::dp::detail

namespace dplx::dp::detail
{

template <typename T>
concept encodable_int = cncr::integer<T> && sizeof(T) <= sizeof(std::uint64_t);

template <typename T>
using encoder_uint_t = std::conditional_t<sizeof(T) <= sizeof(std::uint32_t),
                                          std::uint32_t,
                                          std::uint64_t>;

inline constexpr unsigned indefinite_add_info = 0b000'11111U;

} // namespace dplx::dp::detail

namespace dplx::dp
{

template <detail::encodable_int T>
inline auto emit_integer(emit_context const &ctx, T const value) noexcept
        -> result<void>
{
    using code_type = detail::encoder_uint_t<T>;
    if constexpr (!std::is_signed_v<T>)
    {
        return detail::store_var_uint<code_type>(
                ctx.out, static_cast<code_type>(value), type_code::posint);
    }
    else
    {
        using uvalue_type = std::make_unsigned_t<T>;
        auto const signmask = static_cast<uvalue_type>(
                value >> (detail::digits_v<uvalue_type> - 1U));
        // complement negatives
        uvalue_type const uvalue = signmask ^ static_cast<uvalue_type>(value);

        auto const category = static_cast<type_code>(
                signmask & static_cast<uvalue_type>(type_code::negint));

        return detail::store_var_uint<code_type>(
                ctx.out, static_cast<code_type>(uvalue), category);
    }
}

template <detail::encodable_int T>
inline auto emit_binary(emit_context const &ctx, T const byteSize) noexcept
        -> result<void>
{
    using code_type = detail::encoder_uint_t<T>;
    assert(byteSize >= 0);
    return detail::store_var_uint<code_type>(
            ctx.out, static_cast<code_type>(byteSize), type_code::binary);
}
inline auto emit_binary(emit_context const &ctx,
                        std::byte const *data,
                        std::size_t size) noexcept -> result<void>
{
    DPLX_TRY(detail::store_var_uint<std::size_t>(ctx.out, size,
                                                 type_code::binary));
    return ctx.out.bulk_write(data, size);
}
inline auto emit_binary_indefinite(emit_context const &ctx) noexcept
        -> result<void>
{
    return detail::store_inline_value(ctx.out, detail::indefinite_add_info,
                                      type_code::binary);
}

template <detail::encodable_int T>
inline auto emit_u8string(emit_context const &ctx,
                          T const numCodeUnits) noexcept -> result<void>
{
    using code_type = detail::encoder_uint_t<T>;
    assert(numCodeUnits >= 0);
    return detail::store_var_uint<code_type>(
            ctx.out, static_cast<code_type>(numCodeUnits), type_code::text);
}
inline auto emit_u8string(emit_context const &ctx,
                          char8_t const *data,
                          std::size_t size) noexcept -> result<void>
{
    DPLX_TRY(detail::store_var_uint<std::size_t>(ctx.out, size,
                                                 type_code::text));
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return ctx.out.bulk_write(reinterpret_cast<std::byte const *>(data), size);
}
inline auto emit_u8string(emit_context const &ctx,
                          char const *data,
                          std::size_t size) noexcept -> result<void>
{
    DPLX_TRY(detail::store_var_uint<std::size_t>(ctx.out, size,
                                                 type_code::text));
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return ctx.out.bulk_write(reinterpret_cast<std::byte const *>(data), size);
}
inline auto emit_u8string_indefinite(emit_context const &ctx) noexcept
        -> result<void>
{
    return detail::store_inline_value(ctx.out, detail::indefinite_add_info,
                                      type_code::text);
}

template <detail::encodable_int T>
inline auto emit_array(emit_context const &ctx, T const numElements) noexcept
        -> result<void>
{
    using code_type = detail::encoder_uint_t<T>;
    assert(numElements >= 0);
    return detail::store_var_uint<code_type>(
            ctx.out, static_cast<code_type>(numElements), type_code::array);
}
inline auto emit_array_indefinite(emit_context const &ctx) noexcept
        -> result<void>
{
    return detail::store_inline_value(ctx.out, detail::indefinite_add_info,
                                      type_code::array);
}

template <detail::encodable_int T>
inline auto emit_map(emit_context const &ctx, T const numElements) noexcept
        -> result<void>
{
    using code_type = detail::encoder_uint_t<T>;
    assert(numElements >= 0);
    return detail::store_var_uint<code_type>(
            ctx.out, static_cast<code_type>(numElements), type_code::map);
}
inline auto emit_map_indefinite(emit_context const &ctx) noexcept
        -> result<void>
{
    return detail::store_inline_value(ctx.out, detail::indefinite_add_info,
                                      type_code::map);
}

template <detail::encodable_int T>
inline auto emit_tag(emit_context const &ctx, T const tagValue) noexcept
        -> result<void>
{
    using code_type = detail::encoder_uint_t<T>;
    assert(tagValue >= 0);
    return detail::store_var_uint<code_type>(
            ctx.out, static_cast<code_type>(tagValue), type_code::tag);
}

[[nodiscard]] inline auto emit_boolean(emit_context const &ctx,
                                       bool const value) noexcept
        -> result<void>
{
    return detail::store_inline_value(ctx.out, static_cast<unsigned>(value),
                                      type_code::bool_false);
}

inline auto emit_float_single(emit_context const &ctx,
                              float const value) noexcept -> result<void>
{
    constexpr auto encodedSize = 1U + sizeof(value);
    if (ctx.out.size() < encodedSize) [[unlikely]]
    {
        DPLX_TRY(ctx.out.ensure_size(encodedSize));
    }

    auto *const dest = ctx.out.data();
    *dest = static_cast<std::byte>(type_code::float_single);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    detail::store(dest + 1, value);

    ctx.out.commit_written(encodedSize);
    return oc::success();
}

inline auto emit_float_double(emit_context const &ctx,
                              double const value) noexcept -> result<void>
{
    constexpr auto encodedSize = 1U + sizeof(value);
    if (ctx.out.size() < encodedSize) [[unlikely]]
    {
        DPLX_TRY(ctx.out.ensure_size(encodedSize));
    }

    auto *const dest = ctx.out.data();
    *dest = static_cast<std::byte>(type_code::float_double);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    detail::store(dest + 1, value);

    ctx.out.commit_written(encodedSize);
    return oc::success();
}

inline auto emit_null(emit_context const &ctx) noexcept -> result<void>
{
    return detail::store_inline_value(ctx.out, 0U, type_code::null);
}
inline auto emit_undefined(emit_context const &ctx) noexcept -> result<void>
{
    return detail::store_inline_value(ctx.out, 0U, type_code::undefined);
}
inline auto emit_break(emit_context const &ctx) noexcept -> result<void>
{
    return detail::store_inline_value(ctx.out, 0U, type_code::special_break);
}

} // namespace dplx::dp
