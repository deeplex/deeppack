
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <iterator>
#include <ranges>
#include <stdexcept>

#include <boost/container/small_vector.hpp>

#include <catch2/generators/catch_generators.hpp>

namespace dp_tests
{

namespace gens = Catch::Generators;

inline constexpr std::size_t iterator_generator_sbo_size = 128U;

template <typename T>
class iterator_generator final : public gens::IGenerator<T>
{
public:
    using container_type
            = boost::container::small_vector<T, iterator_generator_sbo_size>;

private:
    container_type mValues;
    typename container_type::size_type mIndex{0U};

public:
    iterator_generator(container_type &&values)
        : mValues(std::move(values))
    {
        if (mValues.empty())
        {
            throw std::runtime_error("iterator_generator must be initialized "
                                     "with at least one value");
        }
    }
    template <std::input_iterator It>
    explicit iterator_generator(It begin, It end)
        : mValues(begin, end)
    {
        if (mValues.empty())
        {
            throw std::runtime_error("iterator_generator must be initialized "
                                     "with at least one value");
        }
    }
    template <std::ranges::input_range R>
        requires std::ranges::common_range<R>
    // NOLINTNEXTLINE(bugprone-forwarding-reference-overload)
    explicit iterator_generator(R &&range)
        : iterator_generator(std::ranges::begin(range), std::ranges::end(range))
    {
    }

    [[nodiscard]] auto get() const -> T const & override
    {
        return mValues.at(mIndex);
    }

    [[nodiscard]] auto next() -> bool override
    {
        mIndex += 1;
        return mIndex < mValues.size();
    }
};

template <std::ranges::input_range R>
    requires std::ranges::common_range<R>
inline auto from_range(R &&range)
        -> gens::GeneratorWrapper<std::ranges::range_value_t<R>>
{
    return {new iterator_generator<std::ranges::range_value_t<R>>(
            static_cast<R &&>(range))};
}
template <typename It>
inline auto from_range(It begin, It end)
        -> gens::GeneratorWrapper<std::iter_value_t<It>>
{
    return {new iterator_generator<std::iter_value_t<It>>(begin, end)};
}

template <std::ranges::borrowed_range R>
    requires std::ranges::input_range<R>
class borrowed_range_generator
    : public gens::IGenerator<std::ranges::range_value_t<R>>
{
    std::ranges::iterator_t<R> mIt;
    std::ranges::sentinel_t<R> mEnd;

public:
    borrowed_range_generator(R &range)
        : mIt(std::ranges::begin(range))
        , mEnd(std::ranges::end(range))
    {
        if (mIt == mEnd)
        {
            throw std::runtime_error("borrowed_range_generator must be "
                                     "initialized with at least one value");
        }
    }

    [[nodiscard]] auto get() const
            -> std::ranges::range_value_t<R> const & override
    {
        return *mIt;
    }

    [[nodiscard]] auto next() -> bool override
    {
        ++mIt;
        return mIt != mEnd;
    }
};

template <std::ranges::input_range R>
    requires std::ranges::borrowed_range<R>
inline auto borrowed_range(R &&range)
        -> gens::GeneratorWrapper<std::ranges::range_value_t<R>>
{
    return {new borrowed_range_generator<R>(range)};
}

} // namespace dp_tests
