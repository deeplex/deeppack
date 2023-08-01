
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/legacy/chunked_output_stream.hpp"

#include <array>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "test_utils.hpp"

namespace dp_tests
{

namespace
{

// NOLINTNEXTLINE(cppcoreguidelines-virtual-class-destructor)
class test_legacy_chunked_output_stream final
    : public dp::legacy::chunked_output_stream_base<
              test_legacy_chunked_output_stream>
{
public:
    friend class dp::legacy::chunked_output_stream_base<
            test_legacy_chunked_output_stream>;
    using base_type = dp::legacy::chunked_output_stream_base<
            test_legacy_chunked_output_stream>;

    std::array<std::vector<std::byte>, 2> mChunks;
    unsigned mNext;

    static constexpr auto partition
            = dp::minimum_guaranteed_write_size * 2U - 1U;

    explicit test_legacy_chunked_output_stream(unsigned int streamSize)
        : base_type({}, streamSize)
        , mChunks()
        , mNext(0U)

    {
        constexpr auto invalidItem = std::byte{0xFEU};
        assert(streamSize > partition);

        mChunks[0].resize(partition);
        std::fill(mChunks[0].begin(), mChunks[0].end(), invalidItem);

        mChunks[1].resize(streamSize - partition);
        std::fill(mChunks[1].begin(), mChunks[1].end(), invalidItem);
    }

private:
    auto acquire_next_chunk_impl() -> result<std::span<std::byte>>
    {
        if (mNext >= mChunks.size())
        {
            return dp::errc::end_of_stream;
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
        return std::span(mChunks[mNext++]);
    }
};

} // namespace

TEST_CASE("legacy_chunked_output_stream smoke tests")
{
    test_legacy_chunked_output_stream subject(
            dp::minimum_guaranteed_write_size * 4 - 1);

    std::array<std::byte, 2> vals{};
    REQUIRE(subject.bulk_write(vals.data(), vals.size()));

    REQUIRE(subject.ensure_size(1U));
    CHECK(subject.size() == subject.partition - 2);
}

} // namespace dp_tests
