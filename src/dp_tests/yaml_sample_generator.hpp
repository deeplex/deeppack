
// Copyright Henrik Steffen Gaßmann 2022
//
// Distributed under the Boost Software License, Version 1.0.
//         (See accompanying file LICENSE or copy at
//           https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstring>
#include <filesystem>
#include <memory>
#include <span>
#include <string_view>

#include <catch2/generators/catch_generator_exception.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <fmt/core.h>
#include <yaml-cpp/yaml.h>

#include "hex_decode.hpp"
#include "item_sample_rt.hpp"
#include "range_generator.hpp" // gens

template <typename T>
struct YAML::convert<dp_tests::item_sample_rt<T>>
{
    static auto decode(Node const &node, dp_tests::item_sample_rt<T> &sample)
            -> bool
    {
        if (!node.IsMap())
        {
            return false;
        }
        auto valueNode = node["value"];
        auto encodedNode = node["encoded"];
        if (!valueNode.IsDefined())
        {
            return false;
        }
        if (!encodedNode.IsScalar())
        {
            return false;
        }

        // decode the value representation
        sample.value = valueNode.template as<T>();

        // decode the hex encodde output binary
        std::string const &hexEncoded = encodedNode.Scalar();
        if (hexEncoded.size() % 2U != 0U
            || !std::ranges::all_of(hexEncoded, dp_tests::is_hex_digit))
        {
            return false;
        }
        sample.encoded = dp_tests::hex_decode(hexEncoded);
        return true;
    }
};

namespace dp_tests
{

class yaml_sample_loader_exception : public Catch::GeneratorException
{
    std::unique_ptr<char[]> mMessage;

public:
    yaml_sample_loader_exception(yaml_sample_loader_exception const &other)
        : yaml_sample_loader_exception(other.mMessage.get())
    {
    }

    explicit yaml_sample_loader_exception(std::string_view message)
        : yaml_sample_loader_exception(alloc_message(message))
    {
    }

private:
    yaml_sample_loader_exception(std::unique_ptr<char[]> message)
        : GeneratorException(message.get())
        , mMessage(std::move(message))
    {
    }

    static auto alloc_message(std::string_view content)
            -> std::unique_ptr<char[]>
    {
        auto allocation
                = std::make_unique_for_overwrite<char[]>(content.size() + 1U);
        std::memcpy(allocation.get(), content.data(), content.size());
        allocation[content.size()] = '\0';
        return allocation;
    }
};

struct equals_string_node
{
    std::string_view value;

    inline auto operator()(decltype(*YAML::const_iterator{}) const &n) const
            -> bool
    {
        return n.first.IsScalar() && n.first.Scalar() == value;
    }
};

template <typename T>
class yaml_sample_loader final : public gens::IGenerator<item_sample_rt<T>>
{
    std::vector<item_sample_rt<T>> mSamples;
    typename std::vector<item_sample_rt<T>>::const_iterator mCurrent;

public:
    explicit yaml_sample_loader(std::filesystem::path const &filename,
                                std::string_view const slice)
    try : mSamples(), mCurrent()
    {
        std::ifstream file(filename);
        file.exceptions(std::ios_base::badbit | std::ios_base::failbit);
        YAML::Node document = YAML::Load(file);
        if (!document.IsMap())
        {
            throw yaml_sample_loader_exception(fmt::format(
                    "sample file [{}] didn't contain a map of test slices",
                    filename.generic_string()));
        }

        auto const docIt = std::find_if(document.begin(), document.end(),
                                        equals_string_node{slice});
        if (docIt == document.end())
        {
            throw yaml_sample_loader_exception(fmt::format(
                    "sample file [{}] didn't contain the test slice [{}]",
                    filename.generic_string(), slice));
        }

        YAML::Node sliceNode = docIt->second;
        if (!sliceNode.IsMap() || sliceNode.size() == 0U)
        {
            throw yaml_sample_loader_exception(fmt::format(
                    "sample file [{}] slice [{}] isn't a map or empty",
                    filename.generic_string(), slice));
        }

        mSamples.reserve(sliceNode.size());
        for (auto const &sample : sliceNode)
        {
            auto &value = mSamples.emplace_back(
                    sample.second.as<item_sample_rt<T>>());
            value.name = sample.first.Scalar();
        }

        mCurrent = mSamples.begin();
    }
    catch (yaml_sample_loader_exception const &)
    {
        // pass through
        throw;
    }
    catch (std::exception const &exc)
    {
        // wrap in order to make generator failure known to Catch framework
        throw yaml_sample_loader_exception(
                fmt::format("yaml_sample_loader failed to load [{}]:\n{}",
                            filename.generic_string(), exc.what()));
    }

    [[nodiscard]] auto get() const -> item_sample_rt<T> const & override
    {
        return *mCurrent;
    }

    [[nodiscard]] auto next() -> bool override
    {
        mCurrent += 1;
        return mCurrent != mSamples.end();
    }
};

template <typename T>
inline auto load_samples_from_yaml(std::filesystem::path const &filename,
                                   std::string_view const slice)
        -> gens::GeneratorWrapper<item_sample_rt<T>>
{
    return {new yaml_sample_loader<T>(filename, slice)};
}

} // namespace dp_tests
