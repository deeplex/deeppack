
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/legacy/chunked_output_stream.hpp"

#include <array>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "blob_matcher.hpp"
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

    explicit test_legacy_chunked_output_stream(unsigned streamSize)
        : base_type({}, streamSize)
        , mChunks()
        , mNext(0U)

    {
        constexpr auto invalidItem = std::byte{0xfeU};
        assert(streamSize > partition);

        mChunks[0].resize(partition);
        std::fill(mChunks[0].begin(), mChunks[0].end(), invalidItem);

        mChunks[1].resize(streamSize - partition);
        std::fill(mChunks[1].begin(), mChunks[1].end(), invalidItem);
    }

    [[nodiscard]] auto content() const -> std::vector<std::byte>
    {
        std::vector<std::byte> result;
        result.reserve(mChunks[0].size() + mChunks[1].size());
        result.insert(result.end(), mChunks[0].begin(), mChunks[0].end());
        result.insert(result.end(), mChunks[1].begin(), mChunks[1].end());
        return result;
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

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
TEST_CASE("legacy_chunked_output_stream wraps correctly with do_grow")
{
    constexpr unsigned streamSize = dp::minimum_guaranteed_write_size * 4 - 1;
    test_legacy_chunked_output_stream subject(streamSize);

    REQUIRE(subject.ensure_size(1U));

    std::byte buffer[streamSize] = {};
    constexpr std::size_t offset
            = test_legacy_chunked_output_stream::partition - 1;
    REQUIRE(subject.bulk_write(std::span(buffer).first(offset)));

    REQUIRE(subject.ensure_size(3U));
    auto *out = subject.data();
    buffer[offset] = out[0] = std::byte{'a'};
    buffer[offset + 1] = out[1] = std::byte{'b'};
    buffer[offset + 2] = out[2] = std::byte{'c'};
    subject.commit_written(3U);

    CHECK(subject.mChunks[0].back() == std::byte{0xfeU});
    REQUIRE(subject.sync_output());
    CHECK(subject.mChunks[0].back() == std::byte{'a'});

    constexpr auto invalidItem = std::byte{0xfeU};
    std::ranges::fill_n(static_cast<std::byte *>(buffer) + offset + 3,
                        streamSize - offset - 3, invalidItem);

    REQUIRE_BLOB_EQ(subject.content(), buffer);
}

TEST_CASE("legacy_chunked_output_stream wraps correctly with bulk_write")
{
    constexpr unsigned streamSize = dp::minimum_guaranteed_write_size * 4;
    test_legacy_chunked_output_stream subject(streamSize);

    std::byte buffer[streamSize] = {};
    constexpr std::size_t offset1 = 2U;
    REQUIRE(subject.bulk_write(std::span(buffer).first(offset1)));
    REQUIRE(subject.ensure_size(test_legacy_chunked_output_stream::partition
                                - 1));
    constexpr auto offset2 = dp::minimum_guaranteed_write_size * 3;

    REQUIRE(subject.bulk_write(std::span(buffer).first(offset2)));

    constexpr auto invalidItem = std::byte{0xfeU};
    std::ranges::fill_n(static_cast<std::byte *>(buffer) + offset1 + offset2,
                        streamSize - (offset1 + offset2), invalidItem);

    REQUIRE_BLOB_EQ(subject.content(), buffer);
}

// NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)

} // namespace dp_tests
