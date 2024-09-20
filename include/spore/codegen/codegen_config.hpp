#pragma once

#include <optional>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"
#include "spore/codegen/codegen_error.hpp"
#include "spore/codegen/codegen_impl.hpp"

namespace spore::codegen
{
    struct codegen_config_step
    {
        std::string name;
        std::string directory;
        std::vector<std::string> templates;
        std::vector<std::string> data;
        std::optional<nlohmann::json> condition;
    };

    struct codegen_config_stage
    {
        std::string name;
        std::string directory;
        std::string parser;
        std::vector<std::string> files;
        std::vector<codegen_config_step> steps;
    };

    struct codegen_config
    {
        std::vector<codegen_config_stage> stages;
    };

    inline void from_json(const nlohmann::json& json, codegen_config_step& value)
    {
        json["name"].get_to(value.name);
        json["directory"].get_to(value.directory);
        json["templates"].get_to(value.templates);

        if (json.contains("condition"))
        {
            json["condition"].get_to(value.condition.emplace());
        }
    }

    inline void from_json(const nlohmann::json& json, codegen_config_stage& value)
    {
        json["name"].get_to(value.name);
        json["directory"].get_to(value.directory);
        json["steps"].get_to(value.steps);

        const nlohmann::json& files = json["files"];
        if (files.is_string())
        {
            files.get_to(value.files.emplace_back());
        }
        else if (files.is_array())
        {
            files.get_to(value.files);
        }

        if (json.contains("parser"))
        {
            json["parser"].get_to(value.parser);
        }
        else
        {
            value.parser = codegen_impl_cpp::name();
        }
    }

    inline void from_json(const nlohmann::json& json, codegen_config& value)
    {
        std::int32_t version = 1;
        if (json.contains("version"))
        {
            json["version"].get_to(version);
        }

        switch (version)
        {
            case 1: {
                json["stages"].get_to(value.stages);
                break;
            }

            default: {
                throw codegen_error(codegen_error_code::configuring, "unknown configuration version {}", version);
            }
        }
    }
}