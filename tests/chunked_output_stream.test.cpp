
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include <dplx/dp/streams/chunked_output_stream.hpp>

#include <cstddef>

#include <array>
#include <vector>

#include "boost-test.hpp"
#include "test_utils.hpp"

namespace dp_tests
{

BOOST_AUTO_TEST_SUITE(streams)

class test_chunked_output_stream final
    : dp::chunked_output_stream_base<test_chunked_output_stream>
{
public:
    friend class dp::chunked_output_stream_base<test_chunked_output_stream>;
    using base_type
            = dp::chunked_output_stream_base<test_chunked_output_stream>;

    std::array<std::vector<std::byte>, 2> mChunks;
    unsigned int mNext;

    explicit test_chunked_output_stream(unsigned int streamSize)
        : base_type({}, streamSize)
        , mChunks()
    {
        constexpr auto invalidItem = std::byte{0xfeu};
        constexpr auto partition = dp::minimum_guaranteed_write_size * 2 - 1;
        assert(streamSize > partition);

        mChunks[0].resize(partition);
        std::fill(mChunks[0].begin(), mChunks[0].end(), invalidItem);

        mChunks[1].resize(streamSize - partition);
        std::fill(mChunks[1].begin(), mChunks[1].end(), invalidItem);
    }
};

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
