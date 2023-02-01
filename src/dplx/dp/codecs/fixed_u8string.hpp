
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <compare>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

#include <dplx/cncr/math_supplement.hpp>
#include <dplx/predef/compiler/clang.h>

#include <dplx/dp/detail/workaround.hpp>
#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/fwd.hpp>

#define DPLX_DP_WORKAROUND_CLANG_BAD_DEFAULT_SPACESHIP                         \
    DPLX_DP_WORKAROUND(DPLX_COMP_CLANG, <, 12, 0, 1)

#if DPLX_DP_WORKAROUND_CLANG_BAD_DEFAULT_SPACESHIP
#include <algorithm>
#endif

namespace dplx::dp
{

template <std::size_t N>
struct fixed_u8string
{
    unsigned mNumCodeUnits;
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays)
    char8_t mCodeUnits[N];

    constexpr fixed_u8string() noexcept
        : mNumCodeUnits{0}
        , mCodeUnits{}
    {
    }

    // funnily enough clang-tidy also flags compiler generated code 😆
    // NOLINTBEGIN(modernize-use-nullptr)
    // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index)

    constexpr fixed_u8string(fixed_u8string const &) noexcept = default;
    constexpr auto operator=(fixed_u8string const &) noexcept
            -> fixed_u8string & = default;

    constexpr fixed_u8string(fixed_u8string &&) noexcept = default;
    constexpr auto operator=(fixed_u8string &&) noexcept
            -> fixed_u8string & = default;

    friend inline constexpr auto operator==(fixed_u8string const &,
                                            fixed_u8string const &) noexcept
            -> bool
            = default;
    friend inline constexpr auto operator<=>(fixed_u8string const &,
                                             fixed_u8string const &) noexcept
            -> std::strong_ordering
            = default;

    // NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)
    // NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
    // NOLINTEND(modernize-use-nullptr)

    // NOLINTBEGIN(cppcoreguidelines-avoid-c-arrays)
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    constexpr fixed_u8string(char8_t const (&codeUnits)[N + 1U]) noexcept
        : mNumCodeUnits{N}
    {
        for (std::size_t i = 0U; i < N; ++i)
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
            mCodeUnits[i] = codeUnits[i];
        }
    }
    // NOLINTEND(cppcoreguidelines-avoid-c-arrays)

    template <std::size_t N2>
    constexpr fixed_u8string(fixed_u8string<N2> const &other) noexcept
        : mNumCodeUnits{static_cast<unsigned>(other.size())}
        , mCodeUnits{}
    {
        static_assert(N2 <= N,
                      "Can only copy import a fixed string of smaller size.");
        for (std::size_t i = 0U; i < N2; ++i)
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
            mCodeUnits[i] = other.mCodeUnits[i];
        }
    }

    friend inline constexpr auto operator==(fixed_u8string const &lhs,
                                            std::u8string_view rhs) noexcept
            -> bool
    {
        return std::u8string_view(lhs) == rhs;
    }
#if DPLX_DP_WORKAROUND_CLANG_BAD_DEFAULT_SPACESHIP

    friend inline constexpr auto operator<=>(fixed_u8string const &lhs,
                                             std::u8string_view rhs) noexcept
            -> std::strong_ordering
    {
        std::u8string_view view(lhs);
        return std::lexicographical_compare_three_way(view.begin(), view.end(),
                                                      rhs.begin(), rhs.end());
    }

#else

    friend inline constexpr auto operator<=>(fixed_u8string const &lhs,
                                             std::u8string_view rhs) noexcept
            -> std::strong_ordering
    {
        return std::u8string_view(lhs) <=> rhs;
    }

#endif

    constexpr operator std::u8string_view() const noexcept
    {
        return {static_cast<char8_t const *>(mCodeUnits), mNumCodeUnits};
    }

    [[nodiscard]] constexpr auto begin() noexcept -> char8_t *
    {
        return static_cast<char8_t *>(mCodeUnits);
    }
    [[nodiscard]] constexpr auto begin() const noexcept -> char8_t const *
    {
        return static_cast<char8_t const *>(mCodeUnits);
    }
    [[nodiscard]] constexpr auto end() noexcept -> char8_t *
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        return static_cast<char8_t *>(mCodeUnits) + mNumCodeUnits;
    }
    [[nodiscard]] constexpr auto end() const noexcept -> char8_t const *
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        return static_cast<char8_t const *>(mCodeUnits) + mNumCodeUnits;
    }
    [[nodiscard]] constexpr auto data() noexcept -> char8_t *
    {
        return static_cast<char8_t *>(mCodeUnits);
    }
    [[nodiscard]] constexpr auto data() const noexcept -> char8_t const *
    {
        return static_cast<char8_t const *>(mCodeUnits);
    }
    [[nodiscard]] constexpr auto empty() const noexcept -> bool
    {
        return mNumCodeUnits == 0U;
    }
    [[nodiscard]] constexpr auto size() const noexcept -> std::size_t
    {
        return mNumCodeUnits;
    }
    [[nodiscard]] static constexpr auto max_size() noexcept -> std::size_t
    {
        return N;
    }
};

template <std::size_t N>
// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays)
fixed_u8string(char8_t const (&)[N])->fixed_u8string<N - 1>;

namespace detail
{

class fixed_u8string_codec_base
{
public:
    static auto size_of(emit_context const &ctx,
                        std::u8string_view value) noexcept -> std::uint64_t;
    static auto encode(emit_context const &ctx,
                       std::u8string_view value) noexcept -> result<void>;
};

auto decode_fixed_u8string(parse_context &ctx, char8_t *out, unsigned &outlen)
        -> result<void>;

} // namespace detail

template <std::size_t N>
class codec<fixed_u8string<N>> : public detail::fixed_u8string_codec_base
{
public:
    static auto decode(parse_context &ctx, fixed_u8string<N> &value) noexcept
            -> result<void>
    {
        value.mNumCodeUnits = static_cast<unsigned>(N);
        return detail::decode_fixed_u8string(
                ctx, static_cast<char8_t *>(value.mCodeUnits),
                value.mNumCodeUnits);
    }
};

} // namespace dplx::dp

template <std::size_t N>
struct std::common_type<dplx::dp::fixed_u8string<N>,
                        dplx::dp::fixed_u8string<N>>
{
    using type = dplx::dp::fixed_u8string<N>;
};

template <std::size_t N1, std::size_t N2>
struct std::common_type<dplx::dp::fixed_u8string<N1>,
                        dplx::dp::fixed_u8string<N2>>
{
    using type = dplx::dp::fixed_u8string<dplx::cncr::round_up_p2(
            N1 < N2 ? N2 : N1,
            // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
            16U)>;
};
