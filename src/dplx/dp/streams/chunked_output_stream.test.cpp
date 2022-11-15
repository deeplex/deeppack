
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/streams/chunked_output_stream.hpp"

#include <array>
#include <cstddef>
#include <vector>

#include "boost-test.hpp"
#include "test_utils.hpp"

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)

namespace dp_tests
{

BOOST_AUTO_TEST_SUITE(streams)

class test_chunked_output_stream final
    : public dp::chunked_output_stream_base<test_chunked_output_stream>
{
public:
    friend class dp::chunked_output_stream_base<test_chunked_output_stream>;
    using base_type
            = dp::chunked_output_stream_base<test_chunked_output_stream>;

    std::array<std::vector<std::byte>, 2> mChunks;
    unsigned mNext;

    explicit test_chunked_output_stream(unsigned int streamSize)
        : base_type({}, streamSize)
        , mChunks()
        , mNext(0U)

    {
        constexpr auto invalidItem = std::byte{0xFEU};
        constexpr auto partition = dp::minimum_guaranteed_write_size * 2U - 1U;
        assert(streamSize > partition);

        mChunks[0].resize(partition);
        std::fill(mChunks[0].begin(), mChunks[0].end(), invalidItem);

        mChunks[1].resize(streamSize - partition);
        std::fill(mChunks[1].begin(), mChunks[1].end(), invalidItem);
    }

private:
    auto acquire_next_chunk_impl(std::uint64_t) -> dp::result<byte_span>
    {
        if (mNext >= mChunks.size())
        {
            return dp::errc::end_of_stream;
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
        return std::span(mChunks[mNext++]);
    }
};

static_assert(dp::output_stream<test_chunked_output_stream>);
static_assert(dp::lazy_output_stream<test_chunked_output_stream>);
static_assert(dp::stream_traits<test_chunked_output_stream>::output);
static_assert(dp::stream_traits<test_chunked_output_stream>::nothrow_output);

struct chunked_output_stream_dependencies
{
    static constexpr unsigned int testSize
            = dp::minimum_guaranteed_write_size * 4 - 1;

    test_chunked_output_stream subject;

    chunked_output_stream_dependencies()
        : subject(testSize)
    {
    }
};

BOOST_FIXTURE_TEST_SUITE(chunked_output_stream,
                         chunked_output_stream_dependencies)

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

} // namespace dp_tests

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
