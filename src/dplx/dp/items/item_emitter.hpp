
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

#include <dplx/dp/detail/item_size.hpp>
#include <dplx/dp/detail/utils.hpp>
#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/streams/output_buffer.hpp>
#include <dplx/dp/type_code.hpp>

namespace dplx::dp::detail
{

template <typename T>
concept encodable_int = cncr::integer<T> && sizeof(T) <= sizeof(std::uint64_t);

template <typename T>
using encoder_uint_t = std::conditional_t<sizeof(T) <= sizeof(std::uint32_t),
                                          std::uint32_t,
                                          std::uint64_t>;

} // namespace dplx::dp::detail

namespace dplx::dp::ng
{

class item_emitter
{
    output_buffer &mOut;

public:
    explicit item_emitter(output_buffer &out) noexcept
        : mOut(out)
    {
    }

private:
    static constexpr unsigned indefinite_add_info = 0b000'11111U;

public:
    template <detail::encodable_int T>
    [[nodiscard]] inline auto integer(T const value) const noexcept
            -> result<void>
    {
        using code_type = detail::encoder_uint_t<T>;
        if constexpr (!std::is_signed_v<T>)
        {
            return store_var_uint<code_type>(static_cast<code_type>(value),
                                             type_code::posint);
        }
        else
        {
            using uvalue_type = std::make_unsigned_t<T>;
            auto const signmask = static_cast<uvalue_type>(
                    value >> (detail::digits_v<uvalue_type> - 1U));
            // complement negatives
            uvalue_type const uvalue
                    = signmask ^ static_cast<uvalue_type>(value);

            auto const category = static_cast<type_code>(
                    signmask & static_cast<uvalue_type>(type_code::negint));

            return store_var_uint<code_type>(static_cast<code_type>(uvalue),
                                             category);
        }
    }

    template <detail::encodable_int T>
    [[nodiscard]] inline auto binary(T const byteSize) const noexcept
            -> result<void>
    {
        using code_type = detail::encoder_uint_t<T>;
        assert(byteSize >= 0);
        return store_var_uint<code_type>(static_cast<code_type>(byteSize),
                                         type_code::binary);
    }
    [[nodiscard]] inline auto binary_indefinite() const noexcept -> result<void>
    {
        return store_inline_value(indefinite_add_info, type_code::binary);
    }

    template <detail::encodable_int T>
    [[nodiscard]] inline auto u8string(T const numCodeUnits) const noexcept
            -> result<void>
    {
        using code_type = detail::encoder_uint_t<T>;
        assert(numCodeUnits >= 0);
        return store_var_uint<code_type>(static_cast<code_type>(numCodeUnits),
                                         type_code::text);
    }
    [[nodiscard]] inline auto u8string_indefinite() const noexcept
            -> result<void>
    {
        return store_inline_value(indefinite_add_info, type_code::text);
    }

    template <detail::encodable_int T>
    [[nodiscard]] inline auto array(T const numElements) const noexcept
            -> result<void>
    {
        using code_type = detail::encoder_uint_t<T>;
        assert(numElements >= 0);
        return store_var_uint<code_type>(static_cast<code_type>(numElements),
                                         type_code::array);
    }
    [[nodiscard]] inline auto array_indefinite() const noexcept -> result<void>
    {
        return store_inline_value(indefinite_add_info, type_code::array);
    }

    template <detail::encodable_int T>
    [[nodiscard]] inline auto map(T const numElements) const noexcept
            -> result<void>
    {
        using code_type = detail::encoder_uint_t<T>;
        assert(numElements >= 0);
        return store_var_uint<code_type>(static_cast<code_type>(numElements),
                                         type_code::map);
    }
    [[nodiscard]] inline auto map_indefinite() const noexcept -> result<void>
    {
        return store_inline_value(indefinite_add_info, type_code::map);
    }

    template <detail::encodable_int T>
    [[nodiscard]] inline auto tag(T const tagValue) const noexcept
            -> result<void>
    {
        using code_type = detail::encoder_uint_t<T>;
        assert(tagValue >= 0);
        return store_var_uint<code_type>(static_cast<code_type>(tagValue),
                                         type_code::tag);
    }

    [[nodiscard]] inline auto boolean(bool const value) const noexcept
            -> result<void>
    {
        return store_inline_value(static_cast<unsigned>(value),
                                  type_code::bool_false);
    }

    [[nodiscard]] inline auto null() const noexcept -> result<void>
    {
        return store_inline_value(0U, type_code::null);
    }
    [[nodiscard]] inline auto undefined() const noexcept -> result<void>
    {
        return store_inline_value(0U, type_code::undefined);
    }
    [[nodiscard]] inline auto break_() const noexcept -> result<void>
    {
        return store_inline_value(0U, type_code::special_break);
    }

    [[nodiscard]] inline auto float_single(float const value) const
            -> result<void>
    {
        constexpr auto encodedSize = 1U + sizeof(value);
        if (mOut.size() < encodedSize) [[unlikely]]
        {
            DPLX_TRY(mOut.ensure_size(encodedSize));
        }

        auto *const dest = mOut.data();
        *dest = static_cast<std::byte>(type_code::float_single);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        detail::store(dest + 1, value);

        mOut.commit_written(encodedSize);
        return oc::success();
    }

    [[nodiscard]] inline auto float_double(double const value) const
            -> result<void>
    {
        constexpr auto encodedSize = 1U + sizeof(value);
        if (mOut.size() < encodedSize) [[unlikely]]
        {
            DPLX_TRY(mOut.ensure_size(encodedSize));
        }

        auto *const dest = mOut.data();
        *dest = static_cast<std::byte>(type_code::float_double);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        detail::store(dest + 1, value);

        mOut.commit_written(encodedSize);
        return oc::success();
    }

private:
    [[nodiscard]] inline auto
    store_inline_value(unsigned const value,
                       type_code const category) const noexcept -> result<void>
    {
        constexpr std::size_t encodedSize = 1U;
        if (mOut.empty()) [[unlikely]]
        {
            DPLX_TRY(mOut.ensure_size(encodedSize));
        }

        *mOut.data() = static_cast<std::byte>(
                value | static_cast<unsigned>(category));

        mOut.commit_written(encodedSize);
        return oc::success();
    }

    template <typename T>
        requires(!std::is_signed_v<T>)
    [[nodiscard]] inline auto
    store_var_uint(T value, type_code const category) const noexcept
            -> result<void>
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

        if (mOut.size() < 1U + sizeof(T)) [[unlikely]]
        {
            return store_var_uint_eos(static_cast<std::uint64_t>(value),
                                      rcategory, bytePowerPlus2, byteSize);
        }

        auto *const dest = mOut.data();
        *dest = static_cast<std::byte>(rcategory + detail::inline_value_max
                                       + bytePowerPlus2 - 1);

        T const encoded = value << (detail::digits_v<T> - bitSize);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        detail::store(dest + 1, encoded);

        mOut.commit_written(byteSize);
        return oc::success();
    }

    [[nodiscard]] auto store_var_uint_eos(std::uint64_t value,
                                          unsigned category,
                                          unsigned bytePowerPlus2,
                                          unsigned byteSize) const noexcept
            -> result<void>
    {
        if (mOut.size() < byteSize) [[unlikely]]
        {
            DPLX_TRY(mOut.ensure_size(byteSize));
        }

        auto *const dest = mOut.data();
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

        mOut.commit_written(byteSize);
        return oc::success();
    }
};

} // namespace dplx::dp::ng
