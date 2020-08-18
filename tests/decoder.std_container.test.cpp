
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include <dplx/dp/decoder/core.hpp>
#include <dplx/dp/decoder/std_container.hpp>

#include <deque>
#include <list>
#include <map>
#include <unordered_map>
#include <vector>

#include <boost/container/deque.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/list.hpp>
#include <boost/container/map.hpp>
#include <boost/container/small_vector.hpp>
#include <boost/container/static_vector.hpp>
#include <boost/container/vector.hpp>
#include <boost/mp11/list.hpp>

#include "boost-test.hpp"
#include "test_input_stream.hpp"
#include "test_utils.hpp"

namespace dp_tests
{

BOOST_AUTO_TEST_SUITE(decoder)

static_assert(dplx::dp::sequence_container<std::deque<int>>);
static_assert(dplx::dp::sequence_container<std::list<int>>);
static_assert(dplx::dp::sequence_container<std::vector<int>>);
static_assert(dplx::dp::sequence_container<boost::container::deque<int>>);
static_assert(dplx::dp::sequence_container<boost::container::list<int>>);
static_assert(
    dplx::dp::sequence_container<boost::container::small_vector<int, 33>>);
static_assert(
    dplx::dp::sequence_container<boost::container::static_vector<int, 37>>);
static_assert(dplx::dp::sequence_container<boost::container::vector<int>>);

static_assert(dplx::dp::associative_container<std::map<int, int>>);
static_assert(dplx::dp::associative_container<std::unordered_map<int, int>>);
static_assert(dplx::dp::associative_container<boost::container::map<int, int>>);
static_assert(
    dplx::dp::associative_container<boost::container::flat_map<int, int>>);

static_assert(dplx::dp::decodable<test_input_stream, std::deque<int>>);
static_assert(dplx::dp::decodable<test_input_stream, std::list<int>>);
static_assert(dplx::dp::decodable<test_input_stream, std::vector<int>>);
static_assert(
    dplx::dp::decodable<test_input_stream, boost::container::deque<int>>);
static_assert(
    dplx::dp::decodable<test_input_stream, boost::container::list<int>>);
static_assert(dplx::dp::decodable<test_input_stream,
                                  boost::container::small_vector<int, 33>>);
static_assert(dplx::dp::decodable<test_input_stream,
                                  boost::container::static_vector<int, 37>>);
static_assert(
    dplx::dp::decodable<test_input_stream, boost::container::vector<int>>);

static_assert(dplx::dp::decodable<test_input_stream, std::map<int, int>>);
static_assert(
    dplx::dp::decodable<test_input_stream, std::unordered_map<int, int>>);
static_assert(
    dplx::dp::decodable<test_input_stream, boost::container::map<int, int>>);
static_assert(dplx::dp::decodable<test_input_stream,
                                  boost::container::flat_map<int, int>>);

BOOST_AUTO_TEST_SUITE(std_container)

using uint_sequence_containers =
    boost::mp11::mp_list<std::deque<unsigned int>,
                         std::list<unsigned int>,
                         std::vector<unsigned int>,
                         boost::container::list<unsigned int>,
                         boost::container::vector<unsigned int>>;

BOOST_AUTO_TEST_CASE_TEMPLATE(empty_array, T, uint_sequence_containers)
{
    auto serializedInput = make_byte_array<12>({0b100'00000});
    test_input_stream stream{byte_span(serializedInput)};

    using decoder_type = dplx::dp::basic_decoder<test_input_stream, T>;

    T out{};
    auto rx = decoder_type()(stream, out);
    DPLX_REQUIRE_RESULT(rx);

    BOOST_TEST(out.size() == 0u);
}
BOOST_AUTO_TEST_CASE_TEMPLATE(empty_indefinite_array,
                              T,
                              uint_sequence_containers)
{
    auto serializedInput = make_byte_array<12>({0b100'11111, 0xFF});
    test_input_stream stream{std::span(serializedInput)};

    using decoder_type = dplx::dp::basic_decoder<test_input_stream, T>;

    T out{};
    auto rx = decoder_type()(stream, out);
    DPLX_REQUIRE_RESULT(rx);

    BOOST_TEST(out.size() == 0u);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(single_item, T, uint_sequence_containers)
{
    auto serializedInput = make_byte_array<12>({0b100'00001, 0x15});
    test_input_stream stream{byte_span(serializedInput)};

    using decoder_type = dplx::dp::basic_decoder<test_input_stream, T>;

    T out{};
    auto rx = decoder_type()(stream, out);
    DPLX_REQUIRE_RESULT(rx);

    BOOST_TEST(out.size() == 1u);
    BOOST_TEST(out.front() == 0x15u);
}
BOOST_AUTO_TEST_CASE_TEMPLATE(single_indefinite_item,
                              T,
                              uint_sequence_containers)
{
    auto serializedInput = make_byte_array<12>({0b100'11111, 0x15, 0xFF});
    test_input_stream stream{byte_span(serializedInput)};

    using decoder_type = dplx::dp::basic_decoder<test_input_stream, T>;

    T out{};
    auto rx = decoder_type()(stream, out);
    DPLX_REQUIRE_RESULT(rx);

    BOOST_TEST(out.size() == 1u);
    BOOST_TEST(out.front() == 0x15u);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(multiple_items, T, uint_sequence_containers)
{
    auto serializedInput =
        make_byte_array<12>({0b100'00000 | 3, 0x15, 0x04, 0x10});
    test_input_stream stream{byte_span(serializedInput)};

    using decoder_type = dplx::dp::basic_decoder<test_input_stream, T>;

    T out{};
    auto rx = decoder_type()(stream, out);
    DPLX_REQUIRE_RESULT(rx);

    BOOST_TEST(out.size() == 3u);
    auto it = out.begin();
    BOOST_TEST(*it++ == 0x15u);
    BOOST_TEST(*it++ == 0x04u);
    BOOST_TEST(*it++ == 0x10u);
}
BOOST_AUTO_TEST_CASE_TEMPLATE(multiple_indefinite_items,
                              T,
                              uint_sequence_containers)
{
    auto serializedInput =
        make_byte_array<12>({0b100'11111, 0x15, 0x04, 0x10, 0xFF});
    test_input_stream stream{byte_span(serializedInput)};

    using decoder_type = dplx::dp::basic_decoder<test_input_stream, T>;

    T out{};
    auto rx = decoder_type()(stream, out);
    DPLX_REQUIRE_RESULT(rx);

    BOOST_TEST(out.size() == 3u);
    auto it = out.begin();
    BOOST_TEST(*it++ == 0x15u);
    BOOST_TEST(*it++ == 0x04u);
    BOOST_TEST(*it++ == 0x10u);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(reject_missing_data, T, uint_sequence_containers)
{
    auto serializedInput = make_byte_array<2>({0b100'00011, 0x15});
    test_input_stream stream{byte_span(serializedInput)};

    using decoder_type = dplx::dp::basic_decoder<test_input_stream, T>;

    T out{};
    auto rx = decoder_type()(stream, out);
    BOOST_TEST(rx.has_error());
    BOOST_TEST(rx.assume_error() == dplx::dp::errc::missing_data);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(reject_missing_items, T, uint_sequence_containers)
{
    auto serializedInput = make_byte_array<4>({0b100'00011, 0x19, 0x04, 0x10});
    test_input_stream stream{byte_span(serializedInput)};

    using decoder_type = dplx::dp::basic_decoder<test_input_stream, T>;

    T out{};
    auto rx = decoder_type()(stream, out);
    BOOST_TEST(rx.has_error());
    BOOST_TEST(rx.assume_error() == dplx::dp::errc::end_of_stream);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(reject_indefinite_item_missing_break,
                              T,
                              uint_sequence_containers)
{
    auto serializedInput = make_byte_array<4>({0b100'11111, 0x15, 0x04, 0x10});
    test_input_stream stream{byte_span(serializedInput)};

    using decoder_type = dplx::dp::basic_decoder<test_input_stream, T>;

    T out{};
    auto rx = decoder_type()(stream, out);
    BOOST_TEST(rx.has_error());
    BOOST_TEST(rx.assume_error() == dplx::dp::errc::end_of_stream);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(reject_invalid_subitems,
                              T,
                              uint_sequence_containers)
{
    auto serializedInput =
        make_byte_array<12>({0b100'11111, 0x15, 0x04, 0x10, 0b001'00000});
    test_input_stream stream{byte_span(serializedInput)};

    using decoder_type = dplx::dp::basic_decoder<test_input_stream, T>;

    T out{};
    auto rx = decoder_type()(stream, out);
    BOOST_TEST(rx.has_error());
    BOOST_TEST(rx.assume_error() == dplx::dp::errc::item_type_mismatch);
}

// BOOST_AUTO_TEST_CASE()

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

} // namespace dp_tests