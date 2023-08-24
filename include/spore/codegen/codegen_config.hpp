#pragma once

#include <optional>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"

#include "spore/codegen/codegen_error.hpp"

namespace spore::codegen
{
    namespace v1
    {
        struct codegen_config_step
        {
            std::string name;
            std::string input;
            std::vector<std::string> templates;
            std::optional<nlohmann::json> condition;
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

            if (json.contains("condition"))
            {
                json["condition"].get_to(value.condition.emplace());
            }
        }

        void from_json(const nlohmann::json& json, codegen_config& value)
        {
            json["steps"].get_to(value.steps);
        }
    }

    struct codegen_config_step
    {
        std::string name;
        std::string directory;
        std::vector<std::string> templates;
        std::optional<nlohmann::json> condition;
    };

    struct codegen_config_stage
    {
        std::string name;
        std::string directory;
        std::string files;
        std::vector<codegen_config_step> steps;
    };

    struct codegen_config
    {
        std::vector<codegen_config_stage> stages;

        static inline codegen_config from_v1(const v1::codegen_config& config_v1)
        {
            const auto transform = [](const v1::codegen_config_step& config_step_v1) {
                codegen_config_stage config_stage;
                config_stage.name = config_step_v1.name;
                config_stage.files = config_step_v1.input;
                config_stage.directory = ".";

                codegen_config_step& config_step = config_stage.steps.emplace_back();
                config_step.name = config_step_v1.name;
                config_step.templates = config_step_v1.templates;
                config_step.condition = config_step_v1.condition;
                config_step.directory = ".";

                return config_stage;
            };

            codegen_config config;
            std::transform(config_v1.steps.begin(), config_v1.steps.end(), std::back_inserter(config.stages), transform);
            return config;
        }
    };

    void from_json(const nlohmann::json& json, codegen_config_step& value)
    {
        json["name"].get_to(value.name);
        json["directory"].get_to(value.directory);
        json["templates"].get_to(value.templates);

        if (json.contains("condition"))
        {
            json["condition"].get_to(value.condition.emplace());
        }
    }

    void from_json(const nlohmann::json& json, codegen_config_stage& value)
    {
        json["name"].get_to(value.name);
        json["files"].get_to(value.files);
        json["directory"].get_to(value.directory);
        json["steps"].get_to(value.steps);
    }

    void from_json(const nlohmann::json& json, codegen_config& value)
    {
        std::uint32_t version = 1;
        if (json.contains("version"))
        {
            json["version"].get_to(version);
        }

        switch (version)
        {
            case 1: {
                v1::codegen_config config_v1;
                v1::from_json(json, config_v1);
                value = codegen_config::from_v1(config_v1);
                break;
            }

            case 2: {
                json["stages"].get_to(value.stages);
                break;
            }

            default: {
                throw codegen_error(codegen_error_code::configuring, "unknown configuration version {}", version);
            }
        }
    }
}