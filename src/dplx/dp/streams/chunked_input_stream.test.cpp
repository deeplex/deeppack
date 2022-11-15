
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/streams/chunked_input_stream.hpp"

#include <array>
#include <cstddef>
#include <vector>

#include "boost-test.hpp"
#include "test_utils.hpp"

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
// NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
// NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index)

namespace dp_tests
{

BOOST_AUTO_TEST_SUITE(streams)

class test_chunked_input_stream final
    : public dp::chunked_input_stream_base<test_chunked_input_stream>
{
public:
    friend class dp::chunked_input_stream_base<test_chunked_input_stream>;
    using base_type = dp::chunked_input_stream_base<test_chunked_input_stream>;

    std::array<std::vector<std::byte>, 2> mChunks;
    unsigned int mNext = 0;

    explicit test_chunked_input_stream(unsigned int streamSize)
        : base_type({}, streamSize)
        , mChunks()
    {
        constexpr auto partition = dp::minimum_guaranteed_read_size * 2 - 1;
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
    auto acquire_next_chunk_impl(std::uint64_t) -> dp::result<dp::memory_view>
    {
        if (mNext >= mChunks.size())
        {
            return dp::errc::end_of_stream;
        }
        return dp::memory_view(std::span(mChunks[mNext++]));
    }
};

static_assert(dp::input_stream<test_chunked_input_stream>);
static_assert(!dp::lazy_input_stream<test_chunked_input_stream>);
static_assert(dp::stream_traits<test_chunked_input_stream>::input);
static_assert(dp::stream_traits<test_chunked_input_stream>::nothrow_input);

struct chunked_input_stream_dependencies
{
    static constexpr std::size_t testSize
            = dp::minimum_guaranteed_read_size * 3 - 1;

    test_chunked_input_stream subject{testSize};
};

BOOST_FIXTURE_TEST_SUITE(chunked_input_stream,
                         chunked_input_stream_dependencies)

BOOST_AUTO_TEST_CASE(correctly_reflects_stream_size)
{
    auto availableRx = dp::available_input_size(subject);
    DPLX_REQUIRE_RESULT(availableRx);

    auto const available = availableRx.assume_value();
    BOOST_TEST(available == testSize);
}

BOOST_AUTO_TEST_CASE(correctly_handles_small_reads)
{
    auto readRx = dp::read(subject, 29);
    DPLX_REQUIRE_RESULT(readRx);
    auto proxy = readRx.assume_value();
    BOOST_TEST(std::ranges::data(proxy) == subject.mChunks[0].data() + 0);
    BOOST_TEST(std::ranges::size(proxy) == 29U);

    BOOST_TEST_REQUIRE(dp::available_input_size(subject).value()
                       == (testSize - 29));

    readRx = dp::read(subject, 31);
    DPLX_REQUIRE_RESULT(readRx);
    proxy = readRx.assume_value();
    BOOST_TEST(std::ranges::data(proxy) == subject.mChunks[0].data() + 29);
    BOOST_TEST(std::ranges::size(proxy) == 31U);

    BOOST_TEST_REQUIRE(dp::available_input_size(subject).value()
                       == (testSize - 29 - 31));

    readRx = dp::read(subject, 7);
    DPLX_REQUIRE_RESULT(readRx);
    proxy = readRx.assume_value();
    BOOST_TEST(std::ranges::data(proxy) == subject.mChunks[0].data() + 60);
    BOOST_TEST(std::ranges::size(proxy) == 7U);

    BOOST_TEST_REQUIRE(dp::available_input_size(subject).value()
                       == (testSize - 29 - 31 - 7));

    readRx = dp::read(subject, 52);
    DPLX_REQUIRE_RESULT(readRx);
    proxy = readRx.assume_value();
    BOOST_TEST(std::ranges::data(proxy) != subject.mChunks[0].data() + 67);
    // 51B = 12B from chunk 0 + 39B from chunk 1 copied to the small buffer
    BOOST_TEST(std::ranges::size(proxy) == 51U);

    BOOST_TEST_REQUIRE(dp::available_input_size(subject).value()
                       == (testSize - 29 - 31 - 7 - 51));

    readRx = dp::read(subject, 1);
    DPLX_REQUIRE_RESULT(readRx);
    proxy = readRx.assume_value();
    BOOST_TEST(std::ranges::data(proxy) == subject.mChunks[1].data() + 39);
    BOOST_TEST(std::ranges::size(proxy) == 1U);

    BOOST_TEST_REQUIRE(dp::available_input_size(subject).value()
                       == (testSize - 29 - 31 - 7 - 51 - 1));

    readRx = dp::read(subject, 1);
    BOOST_TEST_REQUIRE(readRx.has_error());
    BOOST_TEST(readRx.assume_error() == dp::errc::end_of_stream);
}

BOOST_AUTO_TEST_CASE(correctly_handles_reads_with_consumed_breaks_1)
{
    std::array<std::byte, 52> buffer{};

    auto sreadRx = dp::read(subject, 67);
    DPLX_REQUIRE_RESULT(sreadRx);
    auto proxy = sreadRx.assume_value();
    BOOST_TEST(std::ranges::data(proxy) == subject.mChunks[0].data() + 0);
    BOOST_TEST(std::ranges::size(proxy) == 67U);

    BOOST_TEST_REQUIRE(dp::available_input_size(subject).value()
                       == (testSize - 67));

    sreadRx = dp::read(subject, 52);
    DPLX_REQUIRE_RESULT(sreadRx);
    proxy = sreadRx.assume_value();
    BOOST_TEST(std::ranges::data(proxy) != subject.mChunks[0].data() + 67);
    // 51B = 12B from chunk 0 + 39B from chunk 1 copied to the small buffer
    BOOST_TEST(std::ranges::size(proxy) == 51U);
    BOOST_REQUIRE_EQUAL_COLLECTIONS(proxy.begin(), proxy.begin() + 12,
                                    subject.mChunks[0].begin() + 67,
                                    subject.mChunks[0].begin() + 79);
    BOOST_REQUIRE_EQUAL_COLLECTIONS(proxy.begin() + 12, proxy.begin() + 51,
                                    subject.mChunks[1].begin(),
                                    subject.mChunks[1].begin() + 39);

    auto consumeRx = dp::consume(subject, proxy, 10);
    DPLX_REQUIRE_RESULT(consumeRx);

    sreadRx = dp::read(subject, 41);
    DPLX_REQUIRE_RESULT(sreadRx);
    proxy = sreadRx.assume_value();
    BOOST_TEST(std::ranges::data(proxy) != subject.mChunks[0].data() + 77);
    // 51B = 12B from chunk 0 + 39B from chunk 1 copied to the small buffer
    BOOST_TEST(std::ranges::size(proxy) == 41U);
    BOOST_REQUIRE_EQUAL_COLLECTIONS(proxy.begin(), proxy.begin() + 2,
                                    subject.mChunks[0].begin() + 77,
                                    subject.mChunks[0].begin() + 79);
    BOOST_REQUIRE_EQUAL_COLLECTIONS(proxy.begin() + 2, proxy.begin() + 41,
                                    subject.mChunks[1].begin(),
                                    subject.mChunks[1].begin() + 39);

    BOOST_TEST_REQUIRE(dp::available_input_size(subject).value()
                       == (testSize - 29 - 31 - 7 - 51));

    auto readRx = dp::read(subject, buffer.data(), 1);
    DPLX_REQUIRE_RESULT(readRx);
    BOOST_REQUIRE_EQUAL_COLLECTIONS(buffer.begin(), buffer.begin() + 1,
                                    subject.mChunks[1].begin() + 39,
                                    subject.mChunks[1].begin() + 40);

    BOOST_TEST_REQUIRE(dp::available_input_size(subject).value()
                       == (testSize - 29 - 31 - 7 - 51 - 1));

    readRx = dp::read(subject, buffer.data(), 1);
    BOOST_TEST_REQUIRE(readRx.has_error());
    BOOST_TEST(readRx.assume_error() == dp::errc::end_of_stream);
}

BOOST_AUTO_TEST_CASE(correctly_handles_reads_with_consumed_breaks_2)
{
    std::array<std::byte, 52> buffer{};

    auto sreadRx = dp::read(subject, 67);
    DPLX_REQUIRE_RESULT(sreadRx);
    auto proxy = sreadRx.assume_value();
    BOOST_TEST(std::ranges::data(proxy) == subject.mChunks[0].data() + 0);
    BOOST_TEST(std::ranges::size(proxy) == 67U);

    BOOST_TEST_REQUIRE(dp::available_input_size(subject).value()
                       == (testSize - 67));

    sreadRx = dp::read(subject, 52);
    DPLX_REQUIRE_RESULT(sreadRx);
    proxy = sreadRx.assume_value();
    BOOST_TEST(std::ranges::data(proxy) != subject.mChunks[0].data() + 67);
    // 51B = 12B from chunk 0 + 39B from chunk 1 copied to the small buffer
    BOOST_TEST(std::ranges::size(proxy) == 51U);
    BOOST_REQUIRE_EQUAL_COLLECTIONS(proxy.begin(), proxy.begin() + 12,
                                    subject.mChunks[0].begin() + 67,
                                    subject.mChunks[0].begin() + 79);
    BOOST_REQUIRE_EQUAL_COLLECTIONS(proxy.begin() + 12, proxy.begin() + 51,
                                    subject.mChunks[1].begin(),
                                    subject.mChunks[1].begin() + 39);

    auto consumeRx = dp::consume(subject, proxy, 13);
    DPLX_REQUIRE_RESULT(consumeRx);

    sreadRx = dp::read(subject, 38);
    DPLX_REQUIRE_RESULT(sreadRx);
    proxy = sreadRx.assume_value();
    BOOST_TEST(std::ranges::data(proxy) == subject.mChunks[1].data() + 1);
    BOOST_TEST(std::ranges::size(proxy) == 38U);

    BOOST_TEST_REQUIRE(dp::available_input_size(subject).value()
                       == (testSize - 29 - 31 - 7 - 51));

    auto readRx = dp::read(subject, buffer.data(), 1);
    DPLX_REQUIRE_RESULT(readRx);
    BOOST_REQUIRE_EQUAL_COLLECTIONS(buffer.begin(), buffer.begin() + 1,
                                    subject.mChunks[1].begin() + 39,
                                    subject.mChunks[1].begin() + 40);

    BOOST_TEST_REQUIRE(dp::available_input_size(subject).value()
                       == (testSize - 29 - 31 - 7 - 51 - 1));

    readRx = dp::read(subject, buffer.data(), 1);
    BOOST_TEST_REQUIRE(readRx.has_error());
    BOOST_TEST(readRx.assume_error() == dp::errc::end_of_stream);
}

BOOST_AUTO_TEST_CASE(correctly_handles_large_reads)
{
    std::array<std::byte, 52> buffer{};

    auto readRx = dp::read(subject, buffer.data(), 29);
    DPLX_REQUIRE_RESULT(readRx);
    BOOST_REQUIRE_EQUAL_COLLECTIONS(buffer.begin(), buffer.begin() + 29,
                                    subject.mChunks[0].begin(),
                                    subject.mChunks[0].begin() + 29);

    BOOST_TEST_REQUIRE(dp::available_input_size(subject).value()
                       == (testSize - 29));

    readRx = dp::read(subject, buffer.data(), 31);
    DPLX_REQUIRE_RESULT(readRx);
    BOOST_REQUIRE_EQUAL_COLLECTIONS(buffer.begin(), buffer.begin() + 31,
                                    subject.mChunks[0].begin() + 29,
                                    subject.mChunks[0].begin() + 60);

    BOOST_TEST_REQUIRE(dp::available_input_size(subject).value()
                       == (testSize - 29 - 31));

    readRx = dp::read(subject, buffer.data(), 7);
    DPLX_REQUIRE_RESULT(readRx);
    BOOST_REQUIRE_EQUAL_COLLECTIONS(buffer.begin(), buffer.begin() + 7,
                                    subject.mChunks[0].begin() + 60,
                                    subject.mChunks[0].begin() + 67);

    BOOST_TEST_REQUIRE(dp::available_input_size(subject).value()
                       == (testSize - 29 - 31 - 7));

    readRx = dp::read(subject, buffer.data(), 52);
    DPLX_REQUIRE_RESULT(readRx);
    // 51B = 12B from chunk 0 + 39B from chunk 1 copied to the small buffer
    BOOST_REQUIRE_EQUAL_COLLECTIONS(buffer.begin(), buffer.begin() + 12,
                                    subject.mChunks[0].begin() + 67,
                                    subject.mChunks[0].begin() + 79);
    BOOST_REQUIRE_EQUAL_COLLECTIONS(buffer.begin() + 12, buffer.begin() + 52,
                                    subject.mChunks[1].begin(),
                                    subject.mChunks[1].begin() + 40);

    BOOST_TEST_REQUIRE(dp::available_input_size(subject).value()
                       == (testSize - 29 - 31 - 7 - 52));

    readRx = dp::read(subject, buffer.data(), 1);
    BOOST_TEST_REQUIRE(readRx.has_error());
    BOOST_TEST(readRx.assume_error() == dp::errc::end_of_stream);
}

BOOST_AUTO_TEST_CASE(correctly_handles_reads_with_consumed_breaks_3)
{
    std::array<std::byte, 52> buffer{};

    auto sreadRx = dp::read(subject, 67);
    DPLX_REQUIRE_RESULT(sreadRx);
    auto proxy = sreadRx.assume_value();
    BOOST_TEST(std::ranges::data(proxy) == subject.mChunks[0].data() + 0);
    BOOST_TEST(std::ranges::size(proxy) == 67U);

    BOOST_TEST_REQUIRE(dp::available_input_size(subject).value()
                       == (testSize - 67));

    sreadRx = dp::read(subject, 52);
    DPLX_REQUIRE_RESULT(sreadRx);
    proxy = sreadRx.assume_value();
    BOOST_TEST(std::ranges::data(proxy) != subject.mChunks[0].data() + 67);
    // 51B = 12B from chunk 0 + 39B from chunk 1 copied to the small buffer
    BOOST_TEST(std::ranges::size(proxy) == 51U);
    BOOST_REQUIRE_EQUAL_COLLECTIONS(proxy.begin(), proxy.begin() + 12,
                                    subject.mChunks[0].begin() + 67,
                                    subject.mChunks[0].begin() + 79);
    BOOST_REQUIRE_EQUAL_COLLECTIONS(proxy.begin() + 12, proxy.begin() + 51,
                                    subject.mChunks[1].begin(),
                                    subject.mChunks[1].begin() + 39);

    auto consumeRx = dp::consume(subject, proxy, 10);
    DPLX_REQUIRE_RESULT(consumeRx);

    auto readRx = dp::read(subject, buffer.data(), 41);
    DPLX_REQUIRE_RESULT(readRx);
    BOOST_REQUIRE_EQUAL_COLLECTIONS(buffer.begin(), buffer.begin() + 2,
                                    subject.mChunks[0].begin() + 77,
                                    subject.mChunks[0].begin() + 79);
    BOOST_REQUIRE_EQUAL_COLLECTIONS(buffer.begin() + 2, buffer.begin() + 41,
                                    subject.mChunks[1].begin(),
                                    subject.mChunks[1].begin() + 39);

    BOOST_TEST_REQUIRE(dp::available_input_size(subject).value()
                       == (testSize - 29 - 31 - 7 - 51));

    readRx = dp::read(subject, buffer.data(), 1);
    DPLX_REQUIRE_RESULT(readRx);
    BOOST_REQUIRE_EQUAL_COLLECTIONS(buffer.begin(), buffer.begin() + 1,
                                    subject.mChunks[1].begin() + 39,
                                    subject.mChunks[1].begin() + 40);

    BOOST_TEST_REQUIRE(dp::available_input_size(subject).value()
                       == (testSize - 29 - 31 - 7 - 51 - 1));

    readRx = dp::read(subject, buffer.data(), 1);
    BOOST_TEST_REQUIRE(readRx.has_error());
    BOOST_TEST(readRx.assume_error() == dp::errc::end_of_stream);
}

BOOST_AUTO_TEST_CASE(correctly_handles_reads_with_consumed_breaks_4)
{
    std::array<std::byte, 52> buffer{};

    auto sreadRx = dp::read(subject, 67);
    DPLX_REQUIRE_RESULT(sreadRx);
    auto proxy = sreadRx.assume_value();
    BOOST_TEST(std::ranges::data(proxy) == subject.mChunks[0].data() + 0);
    BOOST_TEST(std::ranges::size(proxy) == 67U);

    BOOST_TEST_REQUIRE(dp::available_input_size(subject).value()
                       == (testSize - 67));

    sreadRx = dp::read(subject, 52);
    DPLX_REQUIRE_RESULT(sreadRx);
    proxy = sreadRx.assume_value();
    BOOST_TEST(std::ranges::data(proxy) != subject.mChunks[0].data() + 67);
    // 51B = 12B from chunk 0 + 39B from chunk 1 copied to the small buffer
    BOOST_TEST(std::ranges::size(proxy) == 51U);
    BOOST_REQUIRE_EQUAL_COLLECTIONS(proxy.begin(), proxy.begin() + 12,
                                    subject.mChunks[0].begin() + 67,
                                    subject.mChunks[0].begin() + 79);
    BOOST_REQUIRE_EQUAL_COLLECTIONS(proxy.begin() + 12, proxy.begin() + 51,
                                    subject.mChunks[1].begin(),
                                    subject.mChunks[1].begin() + 39);

    auto consumeRx = dp::consume(subject, proxy, 13);
    DPLX_REQUIRE_RESULT(consumeRx);

    auto readRx = dp::read(subject, buffer.data(), 38);
    DPLX_REQUIRE_RESULT(readRx);
    BOOST_REQUIRE_EQUAL_COLLECTIONS(buffer.begin(), buffer.begin() + 38,
                                    subject.mChunks[1].begin() + 1,
                                    subject.mChunks[1].begin() + 39);

    BOOST_TEST_REQUIRE(dp::available_input_size(subject).value()
                       == (testSize - 29 - 31 - 7 - 51));

    readRx = dp::read(subject, buffer.data(), 1);
    DPLX_REQUIRE_RESULT(readRx);
    BOOST_REQUIRE_EQUAL_COLLECTIONS(buffer.begin(), buffer.begin() + 1,
                                    subject.mChunks[1].begin() + 39,
                                    subject.mChunks[1].begin() + 40);

    BOOST_TEST_REQUIRE(dp::available_input_size(subject).value()
                       == (testSize - 29 - 31 - 7 - 51 - 1));

    readRx = dp::read(subject, buffer.data(), 1);
    BOOST_TEST_REQUIRE(readRx.has_error());
    BOOST_TEST(readRx.assume_error() == dp::errc::end_of_stream);
}

BOOST_AUTO_TEST_CASE(properly_skips_bytes_at_beginning_1)
{
    auto skipRx = dp::skip_bytes(subject, 67);
    DPLX_REQUIRE_RESULT(skipRx);

    auto sreadRx = dp::read(subject, 52);
    DPLX_REQUIRE_RESULT(sreadRx);
    auto proxy = sreadRx.assume_value();
    BOOST_TEST(std::ranges::data(proxy) != subject.mChunks[0].data() + 67);
    // 51B = 12B from chunk 0 + 39B from chunk 1 copied to the small buffer
    BOOST_TEST(std::ranges::size(proxy) == 51U);
    BOOST_REQUIRE_EQUAL_COLLECTIONS(proxy.begin(), proxy.begin() + 12,
                                    subject.mChunks[0].begin() + 67,
                                    subject.mChunks[0].begin() + 79);
    BOOST_REQUIRE_EQUAL_COLLECTIONS(proxy.begin() + 12, proxy.begin() + 51,
                                    subject.mChunks[1].begin(),
                                    subject.mChunks[1].begin() + 39);
}

BOOST_AUTO_TEST_CASE(skips_over_a_chunk_gap)
{
    auto skipRx = dp::skip_bytes(subject, 80);
    DPLX_REQUIRE_RESULT(skipRx);

    auto sreadRx = dp::read(subject, 39);
    DPLX_REQUIRE_RESULT(sreadRx);
    auto proxy = sreadRx.assume_value();
    BOOST_TEST(std::ranges::data(proxy) == subject.mChunks[1].data() + 1);
    // 51B = 12B from chunk 0 + 39B from chunk 1 copied to the small buffer
    BOOST_TEST(std::ranges::size(proxy) == 39U);
}

BOOST_AUTO_TEST_CASE(skips_correctly_from_sbo_1)
{
    auto sreadRx = dp::read(subject, 57);
    DPLX_REQUIRE_RESULT(sreadRx);
    auto proxy = sreadRx.assume_value();
    BOOST_TEST_REQUIRE(std::ranges::size(proxy) == 57U);

    sreadRx = dp::read(subject, 40);
    DPLX_REQUIRE_RESULT(sreadRx);
    proxy = sreadRx.assume_value();
    BOOST_TEST_REQUIRE(std::ranges::size(proxy) == 40U);

    auto consumeRx = dp::consume(subject, proxy, 20U);
    DPLX_REQUIRE_RESULT(consumeRx);

    auto skipRx = dp::skip_bytes(subject, 13U);
    DPLX_REQUIRE_RESULT(skipRx);

    sreadRx = dp::read(subject, 29U);
    DPLX_REQUIRE_RESULT(sreadRx);
    proxy = sreadRx.assume_value();
    BOOST_TEST_REQUIRE(std::ranges::size(proxy) == 29U);
    BOOST_TEST_REQUIRE(std::ranges::data(proxy)
                       == subject.mChunks[1].data() + 11);
}

BOOST_AUTO_TEST_CASE(skips_correctly_from_sbo_2)
{
    auto sreadRx = dp::read(subject, 57);
    DPLX_REQUIRE_RESULT(sreadRx);
    auto proxy = sreadRx.assume_value();
    BOOST_TEST_REQUIRE(std::ranges::size(proxy) == 57U);

    sreadRx = dp::read(subject, 40);
    DPLX_REQUIRE_RESULT(sreadRx);
    proxy = sreadRx.assume_value();
    BOOST_TEST_REQUIRE(std::ranges::size(proxy) == 40U);

    auto consumeRx = dp::consume(subject, proxy, 30U);
    DPLX_REQUIRE_RESULT(consumeRx);

    auto skipRx = dp::skip_bytes(subject, 3U);
    DPLX_REQUIRE_RESULT(skipRx);

    sreadRx = dp::read(subject, 29U);
    DPLX_REQUIRE_RESULT(sreadRx);
    proxy = sreadRx.assume_value();
    BOOST_TEST_REQUIRE(std::ranges::size(proxy) == 29U);
    BOOST_TEST_REQUIRE(std::ranges::data(proxy)
                       == subject.mChunks[1].data() + 11);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

} // namespace dp_tests

// NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)
// NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
