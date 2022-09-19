
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include "dplx/dp/decoder/std_container.hpp"

#include <array>
#include <deque>
#include <list>
#include <map>
#include <set>
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

#include <dplx/predef/compiler.h>

#include <dplx/dp/decoder/core.hpp>

#include "boost-test.hpp"
#include "test_input_stream.hpp"
#include "test_utils.hpp"

template class dplx::dp::basic_decoder<std::array<std::byte, 16>,
                                       dp_tests::test_input_stream>;

namespace dp_tests
{

BOOST_AUTO_TEST_SUITE(decoder)

static_assert(dp::back_insertion_sequence_container<std::deque<int>>);
static_assert(dp::back_insertion_sequence_container<std::list<int>>);
static_assert(dp::back_insertion_sequence_container<std::vector<int>>);
static_assert(
        dp::back_insertion_sequence_container<boost::container::deque<int>>);
static_assert(
        dp::back_insertion_sequence_container<boost::container::list<int>>);
static_assert(dp::back_insertion_sequence_container<
              boost::container::small_vector<int, 33>>);
static_assert(dp::back_insertion_sequence_container<
              boost::container::static_vector<int, 37>>);
static_assert(
        dp::back_insertion_sequence_container<boost::container::vector<int>>);

static_assert(dp::associative_container<std::set<int>>);
static_assert(dp::associative_container<std::map<int, int>>);
static_assert(dp::associative_container<std::unordered_map<int, int>>);
static_assert(dp::associative_container<boost::container::map<int, int>>);
static_assert(dp::associative_container<boost::container::flat_map<int, int>>);

static_assert(dp::decodable<std::deque<int>, test_input_stream>);
static_assert(dp::decodable<std::list<int>, test_input_stream>);
static_assert(dp::decodable<std::vector<int>, test_input_stream>);
static_assert(dp::decodable<boost::container::deque<int>, test_input_stream>);
static_assert(dp::decodable<boost::container::list<int>, test_input_stream>);
static_assert(dp::decodable<boost::container::small_vector<int, 33>,
                            test_input_stream>);
static_assert(dp::decodable<boost::container::static_vector<int, 37>,
                            test_input_stream>);
static_assert(dp::decodable<boost::container::vector<int>, test_input_stream>);

static_assert(dp::decodable<std::set<int>, test_input_stream>);
static_assert(dp::decodable<std::map<int, int>, test_input_stream>);
static_assert(dp::decodable<std::unordered_map<int, int>, test_input_stream>);
static_assert(
        dp::decodable<boost::container::map<int, int>, test_input_stream>);
static_assert(
        dp::decodable<boost::container::flat_map<int, int>, test_input_stream>);

static_assert(dp::decodable<std::array<std::byte, 15>, test_input_stream>);
static_assert(dp::decodable<std::array<std::byte, 16>, test_input_stream>);
static_assert(dp::decodable<std::array<int, 15>, test_input_stream>);
static_assert(dp::decodable<std::array<unsigned, 16>, test_input_stream>);

BOOST_AUTO_TEST_SUITE(std_container)

using uint_sequence_containers
        = boost::mp11::mp_list<std::deque<unsigned int>,
                               std::list<unsigned int>,
                               std::vector<unsigned int>,
                               boost::container::list<unsigned int>,
                               boost::container::vector<unsigned int>>;

BOOST_AUTO_TEST_CASE_TEMPLATE(empty_array, T, uint_sequence_containers)
{
    auto serializedInput = make_byte_array<12>({0b100'00000});
    test_input_stream stream{byte_span(serializedInput)};

    using decoder_type = dp::basic_decoder<T, test_input_stream>;

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

    using decoder_type = dp::basic_decoder<T, test_input_stream>;

    T out{};
    auto rx = decoder_type()(stream, out);
    DPLX_REQUIRE_RESULT(rx);

    BOOST_TEST(out.size() == 0u);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(single_item, T, uint_sequence_containers)
{
    auto serializedInput = make_byte_array<12>({0b100'00001, 0x15});
    test_input_stream stream{byte_span(serializedInput)};

    using decoder_type = dp::basic_decoder<T, test_input_stream>;

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

    using decoder_type = dp::basic_decoder<T, test_input_stream>;

    T out{};
    auto rx = decoder_type()(stream, out);
    DPLX_REQUIRE_RESULT(rx);

    BOOST_TEST(out.size() == 1u);
    BOOST_TEST(out.front() == 0x15u);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(multiple_items, T, uint_sequence_containers)
{
    auto serializedInput
            = make_byte_array<12>({0b100'00000 | 3, 0x15, 0x04, 0x10});
    test_input_stream stream{byte_span(serializedInput)};

    using decoder_type = dp::basic_decoder<T, test_input_stream>;

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
    auto serializedInput
            = make_byte_array<12>({0b100'11111, 0x15, 0x04, 0x10, 0xFF});
    test_input_stream stream{byte_span(serializedInput)};

    using decoder_type = dp::basic_decoder<T, test_input_stream>;

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

    using decoder_type = dp::basic_decoder<T, test_input_stream>;

    T out{};
    auto rx = decoder_type()(stream, out);
    BOOST_TEST(rx.has_error());
    BOOST_TEST(rx.assume_error() == dp::errc::missing_data);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(reject_missing_items, T, uint_sequence_containers)
{
    auto serializedInput = make_byte_array<4>({0b100'00011, 0x19, 0x04, 0x10});
    test_input_stream stream{byte_span(serializedInput)};

    using decoder_type = dp::basic_decoder<T, test_input_stream>;

    T out{};
    auto rx = decoder_type()(stream, out);
    BOOST_TEST(rx.has_error());
    BOOST_TEST(rx.assume_error() == dp::errc::end_of_stream);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(reject_indefinite_item_missing_break,
                              T,
                              uint_sequence_containers)
{
    auto serializedInput = make_byte_array<4>({0b100'11111, 0x15, 0x04, 0x10});
    test_input_stream stream{byte_span(serializedInput)};

    using decoder_type = dp::basic_decoder<T, test_input_stream>;

    T out{};
    auto rx = decoder_type()(stream, out);
    BOOST_TEST(rx.has_error());
    BOOST_TEST(rx.assume_error() == dp::errc::end_of_stream);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(reject_invalid_subitems,
                              T,
                              uint_sequence_containers)
{
    auto serializedInput
            = make_byte_array<12>({0b100'11111, 0x15, 0x04, 0x10, 0b001'00000});
    test_input_stream stream{byte_span(serializedInput)};

    using decoder_type = dp::basic_decoder<T, test_input_stream>;

    T out{};
    auto rx = decoder_type()(stream, out);
    BOOST_TEST_REQUIRE(rx.has_error());
    BOOST_TEST(rx.assume_error() == dp::errc::item_type_mismatch);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

} // namespace dp_tests
