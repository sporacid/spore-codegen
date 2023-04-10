#pragma once

#include <string>
#include <vector>

#include "nlohmann/json.hpp"

namespace spore::codegen
{
    struct codegen_config_step
    {
        std::string name;
        std::string input;
        std::vector<std::string> templates;
    };

    struct codegen_config
    {
        std::vector<codegen_config_step> steps;
    };

    void from_json(const nlohmann::json& json, codegen_config_step& value)
    {
        json["name"].get_to(value.name);
        json["input"].get_to(value.input);
        json["templates"].get_to(value.templates);
    }

    void from_json(const nlohmann::json& json, codegen_config& value)
    {
        json["steps"].get_to(value.steps);
    }
}