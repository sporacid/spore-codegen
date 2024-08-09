#pragma once

#include <memory>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"

#include "spore/codegen/ast/converters/ast_converter.hpp"
#include "spore/codegen/codegen_cache.hpp"

namespace spore::codegen
{
    struct codegen_output_data
    {
        std::size_t id = 0;
        std::size_t template_id = 0;
        std::string path;
        bool matched = false;
    };

    struct codegen_template_data
    {
        std::size_t id = 0;
        std::string path;
        codegen_cache_status status = codegen_cache_status::new_;
    };

    struct codegen_file_data
    {
        std::size_t id = 0;
        std::string path;
        std::vector<codegen_output_data> outputs;
        codegen_cache_status status = codegen_cache_status::new_;
    };

    struct codegen_step_data
    {
        std::size_t id = 0;
        std::vector<codegen_template_data> templates;
        std::shared_ptr<ast_condition> condition;
    };

    struct codegen_stage_data
    {
        std::size_t id = 0;
        std::vector<codegen_file_data> files;
        std::vector<codegen_step_data> steps;
    };

    inline void to_json(nlohmann::json& json, const codegen_output_data& value)
    {
        json["id"] = value.id;
        json["template_id"] = value.template_id;
        json["path"] = value.path;
        json["matched"] = value.matched;
    }

    inline void to_json(nlohmann::json& json, const codegen_template_data& value)
    {
        json["id"] = value.id;
        json["path"] = value.path;
        json["status"] = value.status;
    }

    inline void to_json(nlohmann::json& json, const codegen_file_data& value)
    {
        json["id"] = value.id;
        json["path"] = value.path;
        json["status"] = value.status;
        json["outputs"] = value.outputs;
    }

    inline void to_json(nlohmann::json& json, const codegen_step_data& value)
    {
        json["id"] = value.id;
        json["templates"] = value.templates;
    }

    inline void to_json(nlohmann::json& json, const codegen_stage_data& value)
    {
        json["id"] = value.id;
        json["files"] = value.files;
        json["steps"] = value.steps;
    }
}
