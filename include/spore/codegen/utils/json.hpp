#pragma once

#include <cmath>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"

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
        std::replace(property.begin(), property.end(), '.', '/');

        if (!property.starts_with('/'))
        {
            property.insert(property.begin(), '/');
        }

        nlohmann::json::json_pointer json_ptr {property};
        return json.contains(json_ptr) && truthy(json[json_ptr]);
    }
}
