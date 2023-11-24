
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <ranges>

#include <catch2/internal/catch_move_and_forward.hpp>
#include <catch2/internal/catch_test_macro_impl.hpp>
#include <fmt/core.h>
#include <fmt/ostream.h>
#include <fmt/ranges.h>

namespace dp_tests
{

// clang-format off
template <typename R>
concept blob_like
    = std::ranges::input_range<R> && std::ranges::forward_range<R>
    && (std::is_same_v<std::ranges::range_value_t<R>, std::uint8_t>
        || std::is_same_v<std::ranges::range_value_t<R>, std::byte>
        || std::is_same_v<std::ranges::range_value_t<R>, char>);
// clang-format on

class BlobMatcher
{
    std::ptrdiff_t mMismatchOffset;

public:
    BlobMatcher()
        : mMismatchOffset(-2)
    {
    }

    template <blob_like ArgT, blob_like ExpectedT>
    [[nodiscard]] auto match(ArgT const &arg,
                             ExpectedT const &expected) noexcept -> bool
    {
        auto const [lit, rit]
                = std::ranges::mismatch(arg, expected, byte_equal{});

        mMismatchOffset
                = lit == std::ranges::end(arg)
                                  && rit == std::ranges::end(expected)
                          ? -1
                          : std::ranges::distance(std::ranges::begin(arg), lit);
        return mMismatchOffset == -1;
    }

    [[nodiscard]] inline auto mismatch_position() const noexcept
            -> std::ptrdiff_t
    {
        return mMismatchOffset;
    }

private:
    struct byte_equal
    {
        template <typename LV, typename RV>
        [[nodiscard]] inline auto operator()(LV lv, RV rv) const noexcept
                -> bool
        {
            return static_cast<std::uint8_t>(lv)
                   == static_cast<std::uint8_t>(rv);
        }
    };
};

struct as_printable_ascii
{
    template <typename C>
    [[nodiscard]] inline auto operator()(C const c) const noexcept -> char
    {
        constexpr C isprint_ascii_lower_bound{0x32};
        constexpr C isprint_ascii_upper_bound{0x7E};
        return isprint_ascii_lower_bound <= c && c <= isprint_ascii_upper_bound
                       ? static_cast<char>(c)
                       : '.';
    }
};

template <blob_like ArgT, blob_like ExpectedT>
class BlobMatchExpr final : public Catch::ITransientExpression
{
    ArgT &&mArg;
    ExpectedT &&mExpected;
    BlobMatcher mMatcher;

public:
    inline BlobMatchExpr(ArgT &&arg,
                         ExpectedT &&expected,
                         BlobMatcher blobMatcher)
        : ITransientExpression{true, blobMatcher.match(arg, expected)}
        , mArg(CATCH_FORWARD(arg))
        , mExpected(CATCH_FORWARD(expected))
        , mMatcher(CATCH_MOVE(blobMatcher))
    {
    }

    void streamReconstructedExpression(std::ostream &os) const override
    {
        constexpr auto maxLineLength = CATCH_CONFIG_CONSOLE_WIDTH - 2U;
        constexpr auto layoutWidth = 73U;
        static_assert(
                maxLineLength >= layoutWidth,
                "the blob matcher requires a console width of at least 73.");

        constexpr auto maxDisplayOffset = 4;
        constexpr auto maxDisplayElems = 16;
        auto const mismatchPosition = mMatcher.mismatch_position();
        auto const contextBegin = mismatchPosition >= maxDisplayOffset
                                          ? mismatchPosition - maxDisplayOffset
                                          : 0;
        auto const mismatchOffset = mismatchPosition - contextBegin;

        auto const argDisplayBegin
                = std::ranges::next(std::ranges::begin(mArg), contextBegin);
        auto const argMismatch
                = std::ranges::next(argDisplayBegin, mismatchOffset);
        auto const argDisplayEnd = std::ranges::next(
                argDisplayBegin, maxDisplayElems, std::ranges::end(mArg));
        auto const argNumDisplayed
                = std::ranges::distance(argDisplayBegin, argDisplayEnd);
        auto const argNumSkipped = maxDisplayElems - argNumDisplayed;

        auto const expectedDisplayBegin = std::ranges::next(
                std::ranges::begin(mExpected), contextBegin);
        auto const expectedMismatch
                = std::ranges::next(expectedDisplayBegin, mismatchOffset);
        auto const expectedDisplayEnd
                = std::ranges::next(expectedDisplayBegin, maxDisplayElems,
                                    std::ranges::end(mExpected));
        auto const expectedNumDisplayed = std::ranges::distance(
                expectedDisplayBegin, expectedDisplayEnd);
        auto const expectedNumSkipped = maxDisplayElems - expectedNumDisplayed;

        char strArg[maxDisplayElems + 1] = {};
        std::ranges::transform(argDisplayBegin, argDisplayEnd,
                               static_cast<char *>(strArg),
                               as_printable_ascii{});

        char strExp[maxDisplayElems + 1] = {};
        std::ranges::transform(expectedDisplayBegin, expectedDisplayEnd,
                               static_cast<char *>(strExp),
                               as_printable_ascii{});

        // this is a bit brittle, but Catch isn't exactly helpful in this case…
        auto const prefixAlign = mismatchOffset * 3 + 1;
        fmt::print(os, "pos: {:>{}}{:#06x}\n", "  vv ", prefixAlign + 3,
                   mismatchPosition);
        fmt::print(os, "val: {:>{}}{:02x}  {:>{}}\n", " /", prefixAlign,
                   fmt::join(argMismatch, argDisplayEnd, " "), strArg,
                   argNumSkipped * 3 + argNumDisplayed - 1);
        fmt::print(os, "     {:02x}<\n",
                   fmt::join(argDisplayBegin, argMismatch, " "));
        fmt::print(os, "exp: {:>{}}{:02x}  {:>{}}\n", " \\", prefixAlign,
                   fmt::join(expectedMismatch, expectedDisplayEnd, " "), strExp,
                   expectedNumSkipped * 3 + expectedNumDisplayed - 1);
    }
};

template <typename ArgT, typename ExpectedT>
auto make_blob_match_expr(ArgT &&arg, ExpectedT &&expected)
{
    return ::dp_tests::BlobMatchExpr<ArgT, ExpectedT>(
            CATCH_FORWARD(arg), CATCH_FORWARD(expected),
            ::dp_tests::BlobMatcher{});
}

// NOLINTBEGIN(cppcoreguidelines-macro-usage)

#define INTERNAL_CHECK_BLOB_EQ(macroName, expected, resultDisposition, arg)    \
    do                                                                         \
    {                                                                          \
        Catch::AssertionHandler catchAssertionHandler(                         \
                macroName##_catch_sr, CATCH_INTERNAL_LINEINFO,                 \
                CATCH_INTERNAL_STRINGIFY(arg) ", " CATCH_INTERNAL_STRINGIFY(   \
                        expected),                                             \
                resultDisposition);                                            \
        INTERNAL_CATCH_TRY                                                     \
        {                                                                      \
            catchAssertionHandler.handleExpr(                                  \
                    ::dp_tests::make_blob_match_expr(arg, expected));          \
        }                                                                      \
        INTERNAL_CATCH_CATCH(catchAssertionHandler)                            \
        INTERNAL_CATCH_REACT(catchAssertionHandler)                            \
    }                                                                          \
    while (false)

#define CHECK_BLOB_EQ(arg, expected)                                           \
    INTERNAL_CHECK_BLOB_EQ("CHECK_BLOB_EQ", expected,                          \
                           Catch::ResultDisposition::ContinueOnFailure, arg)
#define REQUIRE_BLOB_EQ(arg, expected)                                         \
    INTERNAL_CHECK_BLOB_EQ("REQUIRE_BLOB_EQ", expected,                        \
                           Catch::ResultDisposition::Normal, arg)

// NOLINTEND(cppcoreguidelines-macro-usage)

} // namespace dp_tests
