
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>
#include <cstdint>

#include <algorithm>
#include <vector>
#include <span>

#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/stream.hpp>

#include "boost-test.hpp"

namespace dp_tests
{

class test_input_stream // #TODO use a validating readproxy
{
    std::span<std::byte const> mBuffer;
    std::vector<std::byte> mReadBuffer;
    std::size_t mStreamPosition;
    int mReadCounter;
    int mCommitCounter;

public:
    explicit test_input_stream(std::span<std::byte const> bs)
        : mBuffer(bs)
        , mReadBuffer()
        , mStreamPosition(0)
        , mReadCounter(0)
        , mCommitCounter(0)
    {
    }

    friend inline auto
    tag_invoke(dplx::dp::tag_t<dplx::dp::available_input_size>,
               test_input_stream &self) noexcept
        -> dplx::dp::result<std::size_t>
    {
        return self.mBuffer.size() - self.mStreamPosition;
    }
    friend inline auto tag_invoke(dplx::dp::tag_t<dplx::dp::read>,
                                  test_input_stream &self,
                                  std::size_t const amount)
        -> dplx::dp::result<std::span<std::byte const>>
    {
        BOOST_TEST(self.mReadCounter == self.mCommitCounter);
        auto const start = self.mStreamPosition;
        if (start + amount > self.mBuffer.size())
        {
            return dplx::dp::errc::end_of_stream;
        }

        self.mReadCounter += 1;
        self.mStreamPosition += amount;

        self.mReadBuffer.resize(amount);
        std::copy_n(
            self.mBuffer.data() + start, amount, self.mReadBuffer.data());

        return std::span(self.mReadBuffer);
    }
    friend inline auto tag_invoke(dplx::dp::tag_t<dplx::dp::consume>,
                                  test_input_stream &self,
                                  std::span<std::byte const> proxy,
                                  std::size_t const actualAmount) noexcept
        -> dplx::dp::result<void>
    {
        BOOST_TEST(proxy.size() >= actualAmount);
        BOOST_TEST(self.mReadCounter == (self.mCommitCounter + 1));
        self.mCommitCounter += 1;
        self.mStreamPosition -= (proxy.size() - actualAmount);
        std::vector<std::byte> tmp;
        self.mReadBuffer.swap(tmp); // dispose the memory
        return dplx::dp::success();
    }
    friend inline auto
    tag_invoke(dplx::dp::tag_t<dplx::dp::consume>,
               test_input_stream &self,
               [[maybe_unused]] std::span<std::byte const> const proxy) noexcept
        -> dplx::dp::result<void>
    {
        BOOST_TEST(self.mReadCounter == (self.mCommitCounter + 1));
        self.mCommitCounter += 1;
        std::vector<std::byte> tmp;
        self.mReadBuffer.swap(tmp); // dispose the memory
        return dplx::dp::success();
    }
    friend inline auto tag_invoke(dplx::dp::tag_t<dplx::dp::read>,
                                  test_input_stream &self,
                                  std::byte *buffer,
                                  std::size_t const amount) noexcept
        -> dplx::dp::result<void>
    {
        BOOST_TEST(self.mReadCounter == self.mCommitCounter);
        if (self.mStreamPosition + amount > self.mBuffer.size())
        {
            return dplx::dp::errc::end_of_stream;
        }

        std::memcpy(buffer, self.mBuffer.data() + self.mStreamPosition, amount);
        self.mStreamPosition += amount;

        return dplx::dp::success();
    }
};
static_assert(dplx::dp::lazy_input_stream<test_input_stream>);

template <typename... Ts>
auto make_test_input_stream(Ts... ts) noexcept -> test_input_stream
{
    static_assert((... && (std::is_integral_v<Ts> || std::is_enum_v<Ts>)));
    return test_input_stream(
        std::vector<std::byte>{static_cast<std::byte>(ts)...});
}

} // namespace dp_tests
