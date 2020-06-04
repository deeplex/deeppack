
// Copyright Henrik Steffen Gaßmann 2020
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#include <dplx/dp/disappointment.hpp>

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

auto error_category_impl::message(int) const -> std::string
{
    using namespace std::string_literals;
    return std::string();
}

error_category_impl const error_category_instance{};

auto error_category() noexcept -> std::error_category const &
{
    return error_category_instance;
}

} // namespace dplx::dp
