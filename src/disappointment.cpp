
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include <dplx/dp/disappointment.hpp>

#include <fmt/format.h>

namespace dplx::dp
{

class error_category_impl : public std::error_category
{
public:
    virtual auto name() const noexcept -> const char * override;
    virtual auto message(int errval) const -> std::string override;
};

auto error_category_impl::name() const noexcept -> const char *
{
    return "dplx::dp error category";
}

auto error_category_impl::message(int code) const -> std::string
{
    using namespace std::string_literals;
    switch (static_cast<errc>(code))
    {
    case errc::nothing:
        return "no error/success"s;
    case errc::end_of_stream:
        return "the input stream is missing data"s;
    case errc::invalid_additional_information:
        return "a CBOR item has been encoded with a reserved/invalid bitsequence"s;
    case errc::item_type_mismatch:
        return "the decoder expected a different CBOR item type"s;
    case errc::item_value_out_of_range:
        return "the CBOR item value over/underflows the target type"s;
    case errc::unknown_property:
        return "the object_utils decoder was fed an unknown map key"s;
    case errc::too_many_properties:
        return "the tuple/object_utils decoder has been fed a CBOR item with more properties than existing property definitions"s;
    case errc::item_version_property_missing:
        return "the encoded tuple/object missed its version property"s;
    case errc::item_version_mismatch:
        return "the encoded tuple/object version is not supported"s;
    case errc::required_object_property_missing:
        return "the encoded object misses a required property"s;
    case errc::not_enough_memory:
        return "not enough memory could be allocated to complete the operation"s;
    case errc::missing_data:
        return "the map/array content cannot fit in the remaining input data"s;
    case errc::invalid_indefinite_subitem:
        return "the indefinite string/binary contained a non-string/binary subitem"s;
    case errc::tuple_size_mismatch:
        return "the tuple utils decoder expected a different number of items"s;

    default:
        return fmt::format(FMT_STRING("unknown code {}"), code);
    }
}

error_category_impl const error_category_instance{};

auto error_category() noexcept -> std::error_category const &
{
    return error_category_instance;
}

} // namespace dplx::dp
