#pragma once

#include <cmath>
#include <cstdint>
#include <map>
#include <ranges>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"

#include "spore/codegen/codegen_error.hpp"

namespace spore::codegen::json
{
    inline bool truthy(const nlohmann::json& json)
    {
        bool truthy = false;
        switch (json.type())
        {
            case nlohmann::json::value_t::string: {
                constexpr char false_string[] {'f', 'a', 'l', 's', 'e'};
                constexpr auto predicate = [](const char c1, const char c2) { return std::tolower(c1) == std::tolower(c2); };
                const std::string& string = json.get<std::string>();
                truthy = !string.empty() && !std::ranges::equal(string, false_string, predicate);
                break;
            }
            case nlohmann::json::value_t::array:
                truthy = !json.get<std::vector<nlohmann::json>>().empty();
                break;
            case nlohmann::json::value_t::object:
                truthy = !json.get<std::map<std::string, nlohmann::json>>().empty();
                break;
            case nlohmann::json::value_t::number_integer:
                truthy = json.get<std::int64_t>() != 0;
                break;
            case nlohmann::json::value_t::number_unsigned:
                truthy = json.get<std::uint64_t>() != 0;
                break;
            case nlohmann::json::value_t::number_float:
                truthy = json.get<std::double_t>() != 0.;
                break;
            case nlohmann::json::value_t::boolean:
                truthy = json.get<bool>();
                break;
            default:
                break;
        }
        return truthy;
    }

    inline bool truthy(const nlohmann::json& json, std::string property)
    {
        std::ranges::replace(property, '.', '/');

        if (!property.starts_with('/'))
        {
            property.insert(property.begin(), '/');
        }

        const nlohmann::json::json_pointer json_ptr {property};
        return json.contains(json_ptr) && truthy(json[json_ptr]);
    }

    template <typename value_t>
    bool get(const nlohmann::json& json, const std::string_view key, value_t& value)
    {
        if (json.contains(key))
        {
            json[key].get_to(value);
            return true;
        }

        return false;
    }

    template <typename value_t, typename default_value_t = value_t>
    void get_opt(const nlohmann::json& json, std::string_view key, value_t& value, const default_value_t& default_value = {})
    {
        if (!get(json, key, value))
        {
            value = default_value;
        }
    }

    template <typename value_t>
    void get_checked(const nlohmann::json& json, std::string_view key, value_t& value, const std::string_view context)
    {
        if (!get(json, key, value)) [[unlikely]]
        {
            throw codegen_error(codegen_error_code::invalid, "JSON property \"{}\" not found in {}", key, context);
        }
    }
}

template <>
struct std::formatter<nlohmann::json>
{
    std::string format_string;

    auto parse(std::format_parse_context& ctx)
    {
        const auto it_end = std::find(ctx.begin(), ctx.end(), '}');
        const std::string_view format_args {ctx.begin(), it_end};

        format_string.resize(0);
        format_string += "{";

        if (not format_args.empty())
        {
            format_string += ":";
            format_string += format_args;
        }

        format_string += "}";

        return it_end;
    }

    auto format(const nlohmann::json& json, std::format_context& ctx) const -> decltype(ctx.out())
    {
        switch (json.type())
        {
            case nlohmann::json::value_t::array: {
                auto out = ctx.out();
                out = std::format_to(out, "[");

                const std::vector<nlohmann::json>& json_values = json.get_ref<const std::vector<nlohmann::json>&>();
                for (const nlohmann::json& json_value : json_values)
                {
                    ctx.advance_to(out);
                    out = format(json_value, ctx);
                }

                out = std::format_to(out, "]");
                return out;
            }

            case nlohmann::json::value_t::string: {
                const std::string& value = json.get_ref<const std::string&>();
                return std::vformat_to(ctx.out(), format_string, std::make_format_args(value));
            }

            case nlohmann::json::value_t::object: {
                const std::string value = json.dump();
                return std::vformat_to(ctx.out(), format_string, std::make_format_args(value));
            }

            case nlohmann::json::value_t::number_integer: {
                const std::int64_t value = json.get<std::int64_t>();
                return std::vformat_to(ctx.out(), format_string, std::make_format_args(value));
            }

            case nlohmann::json::value_t::number_unsigned: {
                const std::uint64_t value = json.get<std::uint64_t>();
                return std::vformat_to(ctx.out(), format_string, std::make_format_args(value));
            }

            case nlohmann::json::value_t::number_float: {
                const std::double_t value = json.get<std::double_t>();
                return std::vformat_to(ctx.out(), format_string, std::make_format_args(value));
            }

            case nlohmann::json::value_t::boolean: {
                const bool value = json.get<bool>();
                return std::vformat_to(ctx.out(), format_string, std::make_format_args(value));
            }

            default:
                return ctx.out();
        }
    }
};
