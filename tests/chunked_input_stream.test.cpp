
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include <dplx/dp/streams/chunked_input_stream.hpp>

#include <cstddef>

#include <array>
#include <vector>

#include "boost-test.hpp"
#include "test_utils.hpp"

namespace dp_tests
{

BOOST_AUTO_TEST_SUITE(streams)

class test_chunked_input_stream final
    : public dplx::dp::chunked_input_stream_base<test_chunked_input_stream>
{
public:
    friend class dplx::dp::chunked_input_stream_base<test_chunked_input_stream>;
    using base_type
            = dplx::dp::chunked_input_stream_base<test_chunked_input_stream>;

    std::array<std::vector<std::byte>, 2> mChunks;
    unsigned int mNext = 0;

    explicit test_chunked_input_stream(unsigned int streamSize)
        : base_type({}, streamSize)
        , mChunks()
    {
        constexpr auto partition
                = dplx::dp::minimum_guaranteed_read_size * 2 - 1;
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
    auto acquire_next_chunk_impl(std::uint64_t)
            -> dplx::dp::result<dplx::dp::const_byte_buffer_view>
    {
        if (mNext >= mChunks.size())
        {
            return dplx::dp::errc::end_of_stream;
        }
        return dplx::dp::const_byte_buffer_view(std::span(mChunks[mNext++]));
    }
};

static_assert(dplx::dp::input_stream<test_chunked_input_stream>);
static_assert(!dplx::dp::lazy_input_stream<test_chunked_input_stream>);
static_assert(dplx::dp::is_zero_copy_capable_v<test_chunked_input_stream>);

struct chunked_input_stream_dependencies
{
    static constexpr std::size_t testSize
            = dplx::dp::minimum_guaranteed_read_size * 3 - 1;

    test_chunked_input_stream subject;

    chunked_input_stream_dependencies()
        : subject(testSize)
    {
    }
};

BOOST_FIXTURE_TEST_SUITE(chunked_input_stream,
                         chunked_input_stream_dependencies)

BOOST_AUTO_TEST_CASE(correctly_reflects_stream_size)
{
    auto availableRx = dplx::dp::available_input_size(subject);
    DPLX_REQUIRE_RESULT(availableRx);

    auto const available = availableRx.assume_value();
    BOOST_TEST(available == testSize);
}

BOOST_AUTO_TEST_CASE(correctly_handles_small_reads)
{
    auto readRx = dplx::dp::read(subject, 29);
    DPLX_REQUIRE_RESULT(readRx);
    auto proxy = readRx.assume_value();
    BOOST_TEST(std::ranges::data(proxy) == subject.mChunks[0].data() + 0);
    BOOST_TEST(std::ranges::size(proxy) == 29u);

    BOOST_TEST_REQUIRE(dplx::dp::available_input_size(subject).value()
                       == (testSize - 29));

    readRx = dplx::dp::read(subject, 31);
    DPLX_REQUIRE_RESULT(readRx);
    proxy = readRx.assume_value();
    BOOST_TEST(std::ranges::data(proxy) == subject.mChunks[0].data() + 29);
    BOOST_TEST(std::ranges::size(proxy) == 31u);

    BOOST_TEST_REQUIRE(dplx::dp::available_input_size(subject).value()
                       == (testSize - 29 - 31));

    readRx = dplx::dp::read(subject, 7);
    DPLX_REQUIRE_RESULT(readRx);
    proxy = readRx.assume_value();
    BOOST_TEST(std::ranges::data(proxy) == subject.mChunks[0].data() + 60);
    BOOST_TEST(std::ranges::size(proxy) == 7u);

    BOOST_TEST_REQUIRE(dplx::dp::available_input_size(subject).value()
                       == (testSize - 29 - 31 - 7));

    readRx = dplx::dp::read(subject, 52);
    DPLX_REQUIRE_RESULT(readRx);
    proxy = readRx.assume_value();
    BOOST_TEST(std::ranges::data(proxy) != subject.mChunks[0].data() + 67);
    BOOST_TEST(std::ranges::size(proxy) == 51u);

    BOOST_TEST_REQUIRE(dplx::dp::available_input_size(subject).value()
                       == (testSize - 29 - 31 - 7 - 51));

    readRx = dplx::dp::read(subject, 1);
    DPLX_REQUIRE_RESULT(readRx);
    proxy = readRx.assume_value();
    BOOST_TEST(std::ranges::data(proxy) == subject.mChunks[1].data() + 39);
    BOOST_TEST(std::ranges::size(proxy) == 1u);

    BOOST_TEST_REQUIRE(dplx::dp::available_input_size(subject).value()
                       == (testSize - 29 - 31 - 7 - 51 - 1));

    readRx = dplx::dp::read(subject, 1);
    BOOST_TEST_REQUIRE(readRx.has_error());
    BOOST_TEST(readRx.assume_error() == dplx::dp::errc::end_of_stream);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

} // namespace dp_tests
