
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
    std::size_t mCurrentSize = 0;
    int mWriteCounter = 0;
    int mCommitCounter = 0;
    std::array<std::byte, MaxSize> mBuffer{};

    enum class ctag
    {
    };

public:
    ~test_output_stream()
    {
        BOOST_TEST(mWriteCounter == mCommitCounter);
    }

    class write_proxy final : public std::span<std::byte>
    {
        std::size_t mInitSize;

        static constexpr std::size_t invalidated_init_size =
            ~static_cast<std::size_t>(0);

    public:
        write_proxy(std::span<std::byte> mem, std::size_t currentSize, ctag)
            : std::span<std::byte>(mem)
            , mInitSize(currentSize)
        {
        }

        using stream_type = test_output_stream;

        friend inline auto tag_invoke(dplx::dp::tag_t<dplx::dp::commit>,
                                      test_output_stream &stream,
                                      write_proxy &self)
            -> dplx::dp::result<void>
        {
            return write_proxy::commit(stream, self);
        }

        friend inline auto tag_invoke(dplx::dp::tag_t<dplx::dp::commit>,
                                      test_output_stream &stream,
                                      write_proxy &self,
                                      std::size_t const actualSize)
            -> dplx::dp::result<void>
        {
            return write_proxy::commit(stream, self, actualSize);
        }

    private:
        static auto commit(test_output_stream &owner, write_proxy &self)
            -> dplx::dp::result<void>
        {
            BOOST_TEST_REQUIRE(self.mInitSize == owner.mCurrentSize);
            self.mInitSize = invalidated_init_size;
            owner.mCommitCounter += 1;
            return dplx::dp::success();
        }

        static auto commit(test_output_stream &owner,
                           write_proxy &self,
                           std::size_t const actualSize)
            -> dplx::dp::result<void>
        {
            BOOST_TEST_REQUIRE(self.mInitSize == owner.mCurrentSize);
            BOOST_TEST_REQUIRE(actualSize <= self.size());

            auto const absoluteSize =
                std::distance(owner.mBuffer.data(), self.data()) + actualSize;
            static_cast<std::span<std::byte> &>(self) = self.first(actualSize);

            owner.mCurrentSize = absoluteSize;
            owner.mCommitCounter += 1;
            self.mInitSize = invalidated_init_size;
            return dplx::dp::success();
        }
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
        BOOST_TEST_REQUIRE(self.mWriteCounter == self.mCommitCounter);

        auto start = self.mCurrentSize;
        self.mCurrentSize += amount;
        BOOST_TEST_REQUIRE(start + amount <= std::ranges::size(self.mBuffer));
        self.mWriteCounter += 1;
        return write_proxy(
            {self.mBuffer.data() + start, amount}, self.mCurrentSize, ctag{});
    }
    friend inline auto tag_invoke(dplx::dp::tag_t<dplx::dp::write>,
                                  test_output_stream &self,
                                  std::byte const *bytes,
                                  std::size_t const amount)
        -> dplx::dp::result<void>
    {
        BOOST_TEST_REQUIRE(self.mWriteCounter == self.mCommitCounter);
        BOOST_TEST_REQUIRE(self.mCurrentSize + amount <=
                           std::ranges::size(self.mBuffer));

        std::memcpy(
            std::ranges::data(self.mBuffer) + self.mCurrentSize, bytes, amount);
        self.mCurrentSize += amount;

        return dplx::dp::success();
    }

    auto data() noexcept -> std::byte *
    {
        return mBuffer.data();
    }
    auto data() const noexcept -> std::byte const *
    {
        return mBuffer.data();
    }
    auto size() const noexcept -> std::size_t
    {
        return mCurrentSize;
    }

    auto write_counter() const noexcept -> int
    {
        return mWriteCounter;
    }
    auto commit_counter() const noexcept -> int
    {
        return mCommitCounter;
    }
};
static_assert(dplx::dp::output_stream<test_output_stream<>>);
static_assert(dplx::dp::lazy_write_proxy<test_output_stream<>::write_proxy>);
static_assert(std::ranges::contiguous_range<test_output_stream<>>);

struct default_encoding_fixture
{
    test_output_stream<> encodingBuffer;
};

} // namespace dp_tests
