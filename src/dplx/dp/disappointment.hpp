
// Copyright Henrik Steffen Gaßmann 2020-2021.
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>
#include <cstdint>
#include <iterator>

#include <dplx/cncr/data_defined_status_domain.hpp>
#include <dplx/cncr/disappointment.hpp>

namespace dplx::dp
{

namespace oc = outcome;

using outcome::failure;
using outcome::success;

// NOLINTNEXTLINE(performance-enum-size)
enum class errc : std::uint32_t
{
    nothing = 0,
    bad,
    end_of_stream,
    invalid_additional_information,
    item_type_mismatch,
    item_value_out_of_range,
    unknown_property,
    too_many_properties,
    item_version_property_missing,
    item_version_mismatch,
    required_object_property_missing,
    not_enough_memory,
    missing_data,
    invalid_indefinite_subitem,
    tuple_size_mismatch,
    duplicate_key,
    oversized_additional_information_coding,
    indefinite_item,
    string_exceeds_size_limit,
    buffer_size_exceeded,

    LIMIT,
};

} // namespace dplx::dp

namespace dplx::cncr
{

template <>
struct status_enum_definition<::dplx::dp::errc>
    : status_enum_definition_defaults<::dplx::dp::errc>
{
    static constexpr char domain_id[] = "F768D4DE-71BF-4A04-B763-51AAA3A8F092";
    static constexpr char domain_name[] = "dplx::dp error domain";

    static constexpr value_descriptor values[] = {
            // clang-format off
        { code::nothing, generic_errc::success,
            "no error/success" },
        { code::bad, generic_errc::unknown,
            "an external API did not meet its operation contract"},
        { code::end_of_stream, generic_errc::unknown,
            "the input stream is missing data" },
        { code::invalid_additional_information, generic_errc::bad_message,
            "a CBOR item has been encoded with a reserved/invalid bitsequence" },
        { code::item_type_mismatch, generic_errc::bad_message,
             "the decoder expected a different CBOR item type" },
        { code::item_value_out_of_range, generic_errc::value_too_large,
            "the CBOR item value over/underflows the target type" },
        { code::unknown_property, generic_errc::bad_message,
            "the object_utils decoder was fed an unknown map key" },
        { code::too_many_properties, generic_errc::bad_message,
            "the tuple/object_utils decoder has been fed a CBOR item with more properties than existing property definitions" },
        { code::item_version_property_missing, generic_errc::bad_message,
            "the encoded tuple/object missed its version property" },
        { code::item_version_mismatch, generic_errc::bad_message,
            "the encoded tuple/object version is not supported" },
        { code::required_object_property_missing, generic_errc::bad_message,
            "the encoded object misses a required property" },
        { code::not_enough_memory, generic_errc::not_enough_memory,
            "not enough memory could be allocated to complete the operation" },
        { code::missing_data, generic_errc::bad_message,
            "the map/array content cannot fit in the remaining input data" },
        { code::invalid_indefinite_subitem, generic_errc::bad_message,
            "the indefinite string/binary contained a non-string/binary subitem" },
        { code::tuple_size_mismatch, generic_errc::bad_message,
            "the tuple utils decoder expected a different number of items" },
        { code::duplicate_key, generic_errc::bad_message,
            "a key appeared a second time during associative container deserialization" },
        { code::oversized_additional_information_coding, generic_errc::bad_message,
            "a CBOR item with a non minimally encoded additional information value has been encountered during canonical or strict parsing" },
        { code::indefinite_item, generic_errc::bad_message,
            "An indefinite binary/string/array/map CBOR item has been encountered during canonical or strict parsing" },
        { code::string_exceeds_size_limit, generic_errc::bad_message,
            "A binary/string CBOR item exceeded a size limit imposed by the user." },
        { code::buffer_size_exceeded, generic_errc::no_buffer_space,
            "The require_input(amount)/ensure_size(amount) call failed due to `amount` exceeding the streams internal buffer size." },
            // clang-format on
    };

    static_assert(std::size(values) == static_cast<std::size_t>(code::LIMIT));
};

} // namespace dplx::cncr

namespace dplx::dp
{

template <typename R>
using result = dplx::result<R>;

}

namespace dplx::dp::detail
{

inline auto try_extract_failure(result<void> in, result<void> &out) -> bool
{
    if (outcome::try_operation_has_value(in))
    {
        return false;
    }
    out = outcome::try_operation_return_as(static_cast<result<void> &&>(in));
    return true;
}
template <cncr::tryable T>
inline auto try_extract_failure(T &&in, result<void> &out) -> bool
{
    if (outcome::try_operation_has_value(in))
    {
        return false;
    }
    out = outcome::try_operation_return_as(static_cast<T &&>(in));
    return true;
}

} // namespace dplx::dp::detail
