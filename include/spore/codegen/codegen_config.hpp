#pragma once

#include <optional>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"

#include "spore/codegen/codegen_error.hpp"
#include "spore/codegen/codegen_impl.hpp"
#include "spore/codegen/utils/json.hpp"

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

    namespace detail
    {
        constexpr std::string_view config_context = "config";
    }

    inline void from_json(const nlohmann::json& json, codegen_config_step& value)
    {
        json::get_checked(json, "name", value.name, detail::config_context);
        json::get_checked(json, "directory", value.directory, detail::config_context);
        json::get_checked(json, "templates", value.templates, detail::config_context);

        nlohmann::json condition;
        if (json::get(json, "condition", condition))
        {
            value.condition = std::move(condition);
        }
    }

    inline void from_json(const nlohmann::json& json, codegen_config_stage& value)
    {
        json::get_checked(json, "name", value.name, detail::config_context);
        json::get_checked(json, "directory", value.directory, detail::config_context);
        json::get_checked(json, "steps", value.steps, detail::config_context);
        json::get_checked(json, "parser", value.parser, detail::config_context);

        nlohmann::json files;
        json::get_opt(json, "files", files);

        if (files.is_string())
        {
            files.get_to(value.files.emplace_back());
        }
        else if (files.is_array())
        {
            files.get_to(value.files);
        }
    }

    inline void from_json(const nlohmann::json& json, codegen_config& value)
    {
        std::int32_t version;
        json::get_opt(json, "version", version, 1);

        switch (version) // NOLINT(*-multiway-paths-covered)
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