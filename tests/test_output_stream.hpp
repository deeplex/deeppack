
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

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

        void shrink(std::size_t const actualSize) noexcept
        {
            BOOST_TEST_REQUIRE(mInitSize == mOwner.mCurrentSize);
            BOOST_TEST_REQUIRE(actualSize <= size());

            auto const absoluteSize =
                std::distance(mOwner.mBuffer.data(), data()) + actualSize;
            static_cast<std::span<std::byte> &>(*this) = first(actualSize);

            mOwner.mCurrentSize = mInitSize = absoluteSize;
        }

    private:
        test_output_stream &mOwner;
        std::size_t mInitSize;
    };

    auto begin() const
    {
        return std::ranges::begin(mBuffer);
    }
    auto end() const
    {
        return std::ranges::begin(mBuffer) + mCurrentSize;
    }

    auto write(std::size_t const amount) -> write_proxy
    {
        auto start = mCurrentSize;
        mCurrentSize += amount;
        BOOST_TEST_REQUIRE(mCurrentSize <= std::ranges::size(mBuffer));
        return write_proxy({mBuffer.data() + start, amount}, *this, ctag{});
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

} // namespace dplx::dp::test_utils
