
// Copyright Henrik Steffen Gaßmann 2020.
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>
#include <cstring>

#include <span>

#include <dplx/dp/disappointment.hpp>
#include <dplx/dp/stream.hpp>

namespace dplx::dp
{

class memory_input_stream
{
    std::span<std::byte const> mBuffer;
    std::size_t mStreamPosition;

public:
    explicit memory_input_stream(
        std::span<std::byte const> messageStream) noexcept
        : mBuffer(messageStream)
        , mStreamPosition(0)
    {
    }

    friend inline auto
    tag_invoke(dplx::dp::tag_t<dplx::dp::available_input_size>,
               memory_input_stream &self) noexcept
        -> dplx::dp::result<std::size_t>
    {
        return self.mBuffer.size() - self.mStreamPosition;
    }
    friend inline auto tag_invoke(dplx::dp::tag_t<dplx::dp::read>,
                                  memory_input_stream &self,
                                  std::size_t const amount) noexcept
        -> dplx::dp::result<std::span<std::byte const>>
    {
        auto const start = self.mStreamPosition;
        if (start + amount > self.mBuffer.size())
        {
            return errc::end_of_stream;
        }

        self.mStreamPosition += amount;

        return self.mBuffer.subspan(start, amount);
    }
    friend inline auto tag_invoke(dplx::dp::tag_t<dplx::dp::consume>,
                                  memory_input_stream &self,
                                  std::span<std::byte const> proxy,
                                  std::size_t const actualAmount) noexcept
        -> dplx::dp::result<void>
    {
        self.mStreamPosition -= (proxy.size() - actualAmount);
        return success();
    }
    friend inline auto tag_invoke(dplx::dp::tag_t<dplx::dp::read>,
                                  memory_input_stream &self,
                                  std::byte *buffer,
                                  std::size_t const amount) noexcept
        -> dplx::dp::result<void>
    {
        if (self.mStreamPosition + amount > self.mBuffer.size())
        {
            return errc::end_of_stream;
        }

        std::memcpy(buffer, self.mBuffer.data() + self.mStreamPosition, amount);
        self.mStreamPosition += amount;

        return success();
    }
};

} // namespace dplx::dp
