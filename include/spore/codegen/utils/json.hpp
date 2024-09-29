#pragma once

#include <cmath>
#include <cstdint>
#include <map>
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
            case nlohmann::json::value_t::string:
                truthy = !json.get<std::string>().empty();
                break;
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
