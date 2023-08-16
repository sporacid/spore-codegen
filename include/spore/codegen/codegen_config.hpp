#pragma once

#include <optional>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"

namespace spore::codegen
{

    struct codegen_config_pass
    {
        std::vector<std::string> templates;
        std::optional<std::string> output;
        std::optional<nlohmann::json> condition;
    };

    struct codegen_config_step
    {
        std::string name;
        std::string input;
        std::vector<codegen_config_pass> passes;
    };

    struct codegen_config
    {
        std::vector<codegen_config_step> steps;
    };

    void from_json(const nlohmann::json& json, codegen_config_pass& value)
    {
        json["templates"].get_to(value.templates);

        if (json.contains("output"))
        {
            json["output"].get_to(value.output.emplace());
        }

        if (json.contains("condition"))
        {
            json["condition"].get_to(value.condition.emplace());
        }
    }

    void from_json(const nlohmann::json& json, codegen_config_step& value)
    {
        json["name"].get_to(value.name);
        json["input"].get_to(value.input);

        if (json.contains("passes"))
        {
            // v2
            json["passes"].get_to(value.passes);
        }
        else
        {
            // v1
            codegen_config_pass& pass = value.passes.emplace_back();
            json["templates"].get_to(pass.templates);

            if (json.contains("condition"))
            {
                json["condition"].get_to(pass.condition.emplace());
            }
        }
    }

    void from_json(const nlohmann::json& json, codegen_config& value)
    {
        json["steps"].get_to(value.steps);
    }
}