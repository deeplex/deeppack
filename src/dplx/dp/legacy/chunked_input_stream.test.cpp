
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/legacy/chunked_input_stream.hpp"

#include <array>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "test_utils.hpp"

namespace dp_tests
{

namespace
{

// NOLINTNEXTLINE(cppcoreguidelines-virtual-class-destructor)
class test_legacy_chunked_input_stream final
    : public dp::legacy::chunked_input_stream_base<
              test_legacy_chunked_input_stream>
{
public:
    friend class dp::legacy::chunked_input_stream_base<
            test_legacy_chunked_input_stream>;
    using base_type = dp::legacy::chunked_input_stream_base<
            test_legacy_chunked_input_stream>;

    std::array<std::vector<std::byte>, 2> mChunks;
    unsigned int mNext = 0;

    static constexpr auto partition
            = (dp::minimum_guaranteed_read_size * 2) - 1;
    explicit test_legacy_chunked_input_stream(unsigned int streamSize)
        : base_type({}, streamSize)
        , mChunks()
    {
        assert(streamSize > partition);

        mChunks[0].resize(partition);
        for (unsigned int i = 0; i < partition; ++i)
        {
            mChunks[0][i] = static_cast<std::byte>(i);
        }
        mChunks[1].resize(streamSize - partition);
        for (unsigned int i = partition; i < streamSize; ++i)
        {
            mChunks[1][i - partition] = static_cast<std::byte>(i);
        }
    }

private:
    auto acquire_next_chunk_impl(std::uint64_t) -> result<dp::memory_view>
    {
        if (mNext >= mChunks.size())
        {
            return dp::errc::end_of_stream;
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
        return dp::memory_view(std::span(mChunks[mNext++]));
    }
};

} // namespace

TEST_CASE("legacy_chunked_input_stream smoke tests")
{
    test_legacy_chunked_input_stream subject(
            (dp::minimum_guaranteed_read_size * 4) - 1);

    std::array<std::byte, 2> vals{};
    REQUIRE(subject.bulk_read(vals.data(), vals.size()));

    REQUIRE(subject.require_input(1U));
    CHECK(subject.size() == subject.partition - 2);
}

} // namespace dp_tests
