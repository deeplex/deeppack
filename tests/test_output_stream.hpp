
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <array>
#include <span>

#include <dplx/dp/concepts.hpp>

#include "boost-test.hpp"

namespace dp_tests
{

template <std::size_t MaxSize = 56>
class test_output_stream final
{
    enum class ctag
    {
    };

public:
    class write_proxy final : public std::span<std::byte>
    {
    public:
        write_proxy(std::span<std::byte> mem, test_output_stream &owner, ctag)
            : std::span<std::byte>(mem)
            , mOwner(owner)
            , mInitSize(owner.mCurrentSize)
        {
        }

        auto commit(std::size_t const actualSize) noexcept
            -> dplx::dp::result<void>
        {
            BOOST_TEST_REQUIRE(mInitSize == mOwner.mCurrentSize);
            BOOST_TEST_REQUIRE(actualSize <= size());

            auto const absoluteSize =
                std::distance(mOwner.mBuffer.data(), data()) + actualSize;
            std::span<std::byte>::operator=(
                std::span<std::byte>::first(actualSize));

            mOwner.mCurrentSize = mInitSize = absoluteSize;
            return dplx::dp::success();
        }

    private:
        test_output_stream &mOwner;
        std::size_t mInitSize;
    };

    auto begin() noexcept
    {
        return std::ranges::begin(mBuffer);
    }
    auto begin() const noexcept
    {
        return std::ranges::begin(mBuffer);
    }
    auto end() noexcept
    {
        return std::ranges::begin(mBuffer) + mCurrentSize;
    }
    auto end() const noexcept
    {
        return std::ranges::begin(mBuffer) + mCurrentSize;
    }

    friend inline auto tag_invoke(dplx::dp::tag_t<dplx::dp::write>,
                                  test_output_stream &self,
                                  std::size_t const amount)
        -> dplx::dp::result<write_proxy>
    {
        auto start = self.mCurrentSize;
        self.mCurrentSize += amount;
        BOOST_TEST_REQUIRE(start + amount <= std::ranges::size(self.mBuffer));
        return write_proxy({self.mBuffer.data() + start, amount}, self, ctag{});
    }
    friend inline auto tag_invoke(dplx::dp::tag_t<dplx::dp::write>,
                                  test_output_stream &self,
                                  std::byte const *bytes,
                                  std::size_t const amount)
        -> dplx::dp::result<void>
    {
        BOOST_TEST_REQUIRE(self.mCurrentSize + amount <=
                           std::ranges::size(self.mBuffer));

        std::memcpy(
            std::ranges::data(self.mBuffer) + self.mCurrentSize, bytes, amount);

        return dplx::dp::success();
    }

    auto data() noexcept -> std::byte *
    {
        return std::ranges::data(mBuffer);
    }
    auto data() const noexcept -> std::byte const *
    {
        return std::ranges::data(mBuffer);
    }
    auto size() const noexcept -> std::size_t
    {
        return mCurrentSize;
    }

private:
    std::size_t mCurrentSize = 0;
    std::array<std::byte, MaxSize> mBuffer{};
};
static_assert(dplx::dp::output_stream<test_output_stream<>>);
static_assert(std::ranges::contiguous_range<test_output_stream<>>);

struct default_encoding_fixture
{
    test_output_stream<> encodingBuffer;
};

} // namespace dp_tests
