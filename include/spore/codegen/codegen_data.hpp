#pragma once

#include <memory>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"

#include "spore/codegen/ast/converters/ast_converter.hpp"
#include "spore/codegen/codegen_cache.hpp"

namespace spore::codegen
{
    struct codegen_template_data
    {
        std::string path;
        std::string output_suffix;
        codegen_cache_status status = codegen_cache_status::new_;
    };

    struct codegen_file_data
    {
        std::string path;
        std::string output_prefix;
        codegen_cache_status status = codegen_cache_status::new_;
    };

    struct codegen_step_data
    {
        std::vector<codegen_template_data> templates;
        std::shared_ptr<ast_condition> condition;
    };

    struct codegen_stage_data
    {
        std::vector<codegen_file_data> files;
        std::vector<codegen_step_data> steps;
    };

    inline void to_json(nlohmann::json& json, const codegen_template_data& value)
    {
        json["path"] = value.path;
        json["status"] = value.status;
        json["output_suffix"] = value.output_suffix;
    }

    inline void to_json(nlohmann::json& json, const codegen_file_data& value)
    {
        json["path"] = value.path;
        json["status"] = value.status;
        json["output_prefix"] = value.output_prefix;
    }

    inline void to_json(nlohmann::json& json, const codegen_step_data& value)
    {
        json["templates"] = value.templates;
    }

    inline void to_json(nlohmann::json& json, const codegen_stage_data& value)
    {
        json["files"] = value.files;
        json["steps"] = value.steps;
    }
}
