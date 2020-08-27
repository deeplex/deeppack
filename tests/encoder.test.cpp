
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include <dplx/dp/encoder/api.hpp>
#include <dplx/dp/encoder/core.hpp>

#include "boost-test.hpp"
#include "encoder.test_utils.hpp"
#include "test_output_stream.hpp"
#include "test_utils.hpp"

namespace
{
struct simple_encodeable_unmoveable
{
    std::byte value;

    simple_encodeable_unmoveable(simple_encodeable_unmoveable const &) = delete;
    auto operator=(simple_encodeable_unmoveable const &)
        -> simple_encodeable_unmoveable & = delete;
    simple_encodeable_unmoveable(simple_encodeable_unmoveable &&) = delete;
    auto operator=(simple_encodeable_unmoveable &&)
        -> simple_encodeable_unmoveable & = delete;
    simple_encodeable_unmoveable() = default;
    simple_encodeable_unmoveable(std::byte v)
        : value(v)
    {
    }
};
static_assert(std::is_same_v<simple_encodeable_unmoveable const &,
                             dplx::dp::detail::select_proper_param_type<
                                 simple_encodeable_unmoveable>>);
} // namespace

namespace dplx::dp
{
template <output_stream Stream>
class basic_encoder<simple_encodeable_unmoveable, Stream>
{
public:
    auto operator()(Stream &outStream, simple_encodeable_unmoveable const &x) const
        -> result<void>
    {
        using simple = dp_tests::simple_encodeable;
        return basic_encoder<simple, Stream>()(outStream, simple{x.value});
    }
};
} // namespace dplx::dp

namespace dp_tests
{

BOOST_FIXTURE_TEST_SUITE(encoder, default_encoding_fixture)

static_assert(dplx::dp::pair_like<dplx::dp::map_pair<int, int>>);
static_assert(std::is_trivial_v<dplx::dp::map_pair<int, int>>);


static_assert(!dplx::dp::encodable<volatile int, test_output_stream<>>);
static_assert(!dplx::dp::encodable<volatile int const, test_output_stream<>>);
static_assert(!dplx::dp::encodable<char, test_output_stream<>>);

// the integer encoder template just forwards to typ_encoder::integer()
// which is already covered by the item_emitter test suite
static_assert(dplx::dp::encodable<signed char, test_output_stream<>>);
static_assert(dplx::dp::encodable<short, test_output_stream<>>);
static_assert(dplx::dp::encodable<int, test_output_stream<>>);
static_assert(dplx::dp::encodable<long, test_output_stream<>>);
static_assert(dplx::dp::encodable<long long, test_output_stream<>>);

static_assert(dplx::dp::encodable<unsigned char, test_output_stream<>>);
static_assert(dplx::dp::encodable<unsigned short, test_output_stream<>>);
static_assert(dplx::dp::encodable<unsigned int, test_output_stream<>>);
static_assert(dplx::dp::encodable<unsigned long, test_output_stream<>>);
static_assert(dplx::dp::encodable<unsigned long long, test_output_stream<>>);

BOOST_AUTO_TEST_SUITE(encode_api)

BOOST_AUTO_TEST_CASE(simple_dispatch)
{
    simple_encodeable testValue{std::byte{0b1110'1001}};

    DPLX_TEST_RESULT(dplx::dp::encode(encodingBuffer, testValue));

    BOOST_TEST(encodingBuffer.size() == 1u);
    BOOST_TEST(encodingBuffer.data()[0] == testValue.value);
}
BOOST_AUTO_TEST_CASE(simple_bind_dispatch)
{
    simple_encodeable testValue{std::byte{0b1110'1001}};

    DPLX_TEST_RESULT(dplx::dp::encode.bind(encodingBuffer)(testValue));

    BOOST_TEST(encodingBuffer.size() == 1u);
    BOOST_TEST(encodingBuffer.data()[0] == testValue.value);
}
BOOST_AUTO_TEST_CASE(simple_bind_type)
{
    simple_encodeable_unmoveable const testValue{std::byte{0b1110'1001}};

    DPLX_TEST_RESULT(dplx::dp::encode.bind<simple_encodeable_unmoveable>(
        encodingBuffer)(testValue));

    BOOST_TEST(encodingBuffer.size() == 1u);
    BOOST_TEST(encodingBuffer.data()[0] == testValue.value);
}

BOOST_AUTO_TEST_CASE(array_dispatch)
{
    auto encoded =
        make_byte_array(to_byte(dplx::dp::type_code::array) | std::byte{2},
                        std::byte{0b1110'1001},
                        std::byte{0b1110'1001});

    simple_encodeable const lvalue{encoded[1]};
    DPLX_TEST_RESULT(dplx::dp::encode_array(
        encodingBuffer, lvalue, simple_encodeable{encoded[2]}));

    BOOST_TEST(std::span(encodingBuffer) == encoded,
               boost::test_tools::per_element{});
}
BOOST_AUTO_TEST_CASE(array_bind)
{
    auto encoded =
        make_byte_array(to_byte(dplx::dp::type_code::array) | std::byte{2},
                        std::byte{0b1110'1001},
                        std::byte{0b1110'1001});

    simple_encodeable lvalue{encoded[1]};
    DPLX_TEST_RESULT(dplx::dp::encode_array.bind(encodingBuffer)(
        lvalue, simple_encodeable{encoded[2]}));

    BOOST_TEST(std::span(encodingBuffer) == encoded,
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE(map_dispatch)
{
    auto encoded =
        make_byte_array(to_byte(dplx::dp::type_code::map) | std::byte{2},
                        std::byte{0},
                        dplx::dp::type_code::null,
                        std::byte{1},
                        dplx::dp::type_code::null);

    std::pair<simple_encodeable_unmoveable, dplx::dp::null_type> lvalue{};
    DPLX_TEST_RESULT(dplx::dp::encode_map(
        encodingBuffer, lvalue, dplx::dp::map_pair{1, dplx::dp::null_value}));

    BOOST_TEST(std::span(encodingBuffer) == encoded,
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE(map_dispatch_noncopyable)
{
    auto encoded =
        make_byte_array(to_byte(dplx::dp::type_code::map) | std::byte{2},
                        std::byte{0},
                        dplx::dp::type_code::null,
                        std::byte{1},
                        dplx::dp::type_code::null);

    std::pair lvalue{0, dplx::dp::null_value};
    DPLX_TEST_RESULT(dplx::dp::encode_map(
        encodingBuffer, lvalue, std::pair{1, dplx::dp::null_value}));

    BOOST_TEST(std::span(encodingBuffer) == encoded,
               boost::test_tools::per_element{});
}
BOOST_AUTO_TEST_CASE(map_bind)
{
    auto encoded =
        make_byte_array(to_byte(dplx::dp::type_code::map) | std::byte{2},
                        std::byte{0},
                        dplx::dp::type_code::null,
                        std::byte{1},
                        dplx::dp::type_code::null);

    std::pair const lvalue{0, dplx::dp::null_value};
    DPLX_TEST_RESULT(dplx::dp::encode_map.bind(encodingBuffer)(
        lvalue, std::pair{1, dplx::dp::null_value}));

    BOOST_TEST(std::span(encodingBuffer) == encoded,
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE(varargs_0)
{
    DPLX_TEST_RESULT(dplx::dp::encode_varargs(encodingBuffer));

    BOOST_TEST(encodingBuffer.size() == 0u);
}
BOOST_AUTO_TEST_CASE(varargs_1)
{
    auto encoded = make_byte_array(dplx::dp::type_code::null);

    DPLX_TEST_RESULT(
        dplx::dp::encode_varargs(encodingBuffer, dplx::dp::null_value));

    BOOST_TEST(std::span(encodingBuffer) == encoded,
               boost::test_tools::per_element{});
}
BOOST_AUTO_TEST_CASE(varargs_2)
{
    auto encoded =
        make_byte_array(to_byte(dplx::dp::type_code::array) | std::byte{2},
                        std::byte{1},
                        std::byte{0});

    long lvalue{1};
    DPLX_TEST_RESULT(dplx::dp::encode_varargs(encodingBuffer, lvalue, 0));

    BOOST_TEST(std::span(encodingBuffer) == encoded,
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE(varargs_0_bind)
{
    DPLX_TEST_RESULT(dplx::dp::encode_varargs.bind(encodingBuffer)());

    BOOST_TEST(encodingBuffer.size() == 0u);
}
BOOST_AUTO_TEST_CASE(varargs_1_bind)
{
    auto encoded = make_byte_array(dplx::dp::type_code::null);

    DPLX_TEST_RESULT(
        dplx::dp::encode_varargs(encodingBuffer, dplx::dp::null_value));

    BOOST_TEST(std::span(encodingBuffer) == encoded,
               boost::test_tools::per_element{});
}
BOOST_AUTO_TEST_CASE(varargs_2_bind)
{
    auto encoded =
        make_byte_array(to_byte(dplx::dp::type_code::array) | std::byte{2},
                        std::byte{1},
                        std::byte{0});

    long const lvalue{1};
    DPLX_TEST_RESULT(dplx::dp::encode_varargs.bind(encodingBuffer)(lvalue, 0));

    BOOST_TEST(std::span(encodingBuffer) == encoded,
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_CASE(bool_false)
{
    using test_encoder = dplx::dp::basic_encoder<bool, test_output_stream<>>;
    DPLX_TEST_RESULT(test_encoder()(encodingBuffer,false));

    BOOST_TEST(encodingBuffer.size() == 1u);
    BOOST_TEST(encodingBuffer.data()[0] == dplx::dp::type_code::bool_false);
}
BOOST_AUTO_TEST_CASE(bool_true)
{
    using test_encoder = dplx::dp::basic_encoder<bool, test_output_stream<>>;
    DPLX_TEST_RESULT(test_encoder()(encodingBuffer,true));

    BOOST_TEST(encodingBuffer.size() == 1u);
    BOOST_TEST(encodingBuffer.data()[0] == dplx::dp::type_code::bool_true);
}

BOOST_AUTO_TEST_CASE(null_value)
{
    using test_encoder =
        dplx::dp::basic_encoder<dplx::dp::null_type, test_output_stream<>>;
    DPLX_TEST_RESULT(test_encoder()(encodingBuffer,dplx::dp::null_value));

    BOOST_TEST(encodingBuffer.size() == 1u);
    BOOST_TEST(encodingBuffer.data()[0] == dplx::dp::type_code::null);
}

BOOST_AUTO_TEST_CASE(float_api)
{
    using test_encoder = dplx::dp::basic_encoder<float, test_output_stream<>>;
    DPLX_TEST_RESULT(test_encoder()(encodingBuffer,100000.0f));

    auto encodedValue = make_byte_array(0x47, 0xc3, 0x50, 0x00);
    BOOST_TEST_REQUIRE(encodingBuffer.size() == encodedValue.size() + 1);
    BOOST_TEST(encodingBuffer.data()[0] == dplx::dp::type_code::float_single);

    BOOST_TEST(std::span(encodingBuffer).subspan(1) == encodedValue,
               boost::test_tools::per_element{});
}
BOOST_AUTO_TEST_CASE(double_api)
{
    using test_encoder = dplx::dp::basic_encoder<double, test_output_stream<>>;
    DPLX_TEST_RESULT(test_encoder()(encodingBuffer,1.1));

    auto encodedValue =
        make_byte_array(0x3f, 0xf1, 0x99, 0x99, 0x99, 0x99, 0x99, 0x9a);
    BOOST_TEST_REQUIRE(encodingBuffer.size() == encodedValue.size() + 1);
    BOOST_TEST(encodingBuffer.data()[0] == dplx::dp::type_code::float_double);

    BOOST_TEST(std::span(encodingBuffer).subspan(1) == encodedValue,
               boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_CASE(void_dispatch_api)
{
    using test_encoder = dplx::dp::basic_encoder<void, test_output_stream<>>;
    DPLX_TEST_RESULT(test_encoder()(encodingBuffer,dplx::dp::null_value));

    BOOST_TEST(encodingBuffer.size() == 1u);
    BOOST_TEST(encodingBuffer.data()[0] == dplx::dp::type_code::null);
}

BOOST_AUTO_TEST_CASE(vararg_dispatch_0)
{
    using test_encoder =
        dplx::dp::basic_encoder<dplx::dp::mp_varargs<>, test_output_stream<>>;
    DPLX_TEST_RESULT(test_encoder()(encodingBuffer));

    BOOST_TEST(encodingBuffer.size() == 1u);
    BOOST_TEST(encodingBuffer.data()[0] == dplx::dp::type_code::array);
}
BOOST_AUTO_TEST_CASE(vararg_dispatch_1)
{
    using test_encoder =
        dplx::dp::basic_encoder<dplx::dp::mp_varargs<dplx::dp::null_type>, test_output_stream<>>;
    DPLX_TEST_RESULT(test_encoder()(encodingBuffer,dplx::dp::null_value));

    BOOST_TEST(encodingBuffer.size() == 2u);
    BOOST_TEST(encodingBuffer.data()[0] ==
               (to_byte(dplx::dp::type_code::array) | std::byte{1}));
    BOOST_TEST(encodingBuffer.data()[1] == dplx::dp::type_code::null);
}
BOOST_AUTO_TEST_CASE(vararg_dispatch_2)
{
    using test_encoder =
        dplx::dp::basic_encoder<dplx::dp::mp_varargs<dplx::dp::null_type, int>, test_output_stream<>>;
    DPLX_TEST_RESULT(test_encoder()(encodingBuffer,dplx::dp::null_value, 0));

    BOOST_TEST(encodingBuffer.size() == 3u);
    BOOST_TEST(encodingBuffer.data()[0] ==
               (to_byte(dplx::dp::type_code::array) | std::byte{2}));
    BOOST_TEST(encodingBuffer.data()[1] == dplx::dp::type_code::null);
    BOOST_TEST(encodingBuffer.data()[2] == dplx::dp::type_code::posint);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace dp_tests
