#pragma once

#include <string>
#include <vector>

#include "nlohmann/json.hpp"

#include "spore/codegen/codegen_cache.hpp"

namespace spore::codegen
{
    struct codegen_output_data
    {
        std::size_t step_index = 0;
        std::size_t template_index = 0;
        std::string path;
        bool matched = false;
    };

    struct codegen_template_data
    {
        std::string path;
        codegen_cache_status status = codegen_cache_status::new_;
    };

    struct codegen_file_data
    {
        std::string path;
        std::vector<codegen_output_data> outputs;
        codegen_cache_status status = codegen_cache_status::new_;
    };

    struct codegen_step_data
    {
        std::vector<std::size_t> template_indices;
        std::optional<nlohmann::json> condition;
    };

    struct codegen_stage_data
    {
        std::vector<codegen_file_data> files;
        std::vector<codegen_step_data> steps;
    };

    struct codegen_data
    {
        using stage_func_t = std::function<codegen_stage_data()>;
        std::vector<stage_func_t> stage_funcs;
        std::vector<codegen_template_data> templates;

        [[nodiscard]] codegen_stage_data make_stage(const std::size_t index) const
        {
            stage_func_t stage_func = stage_funcs.at(index);
            return stage_func();
        }
    };

    inline void to_json(nlohmann::json& json, const codegen_output_data& value)
    {
        json["template_index"] = value.template_index;
        json["path"] = value.path;
        json["matched"] = value.matched;
    }

    inline void to_json(nlohmann::json& json, const codegen_template_data& value)
    {
        json["path"] = value.path;
        json["status"] = value.status;
    }

    inline void to_json(nlohmann::json& json, const codegen_file_data& value)
    {
        json["path"] = value.path;
        json["status"] = value.status;
        json["outputs"] = value.outputs;
    }

    inline void to_json(nlohmann::json& json, const codegen_step_data& value)
    {
        json["template_indices"] = value.template_indices;

        if (value.condition.has_value())
        {
            json["condition"] = value.condition.value();
        }
    }

    inline void to_json(nlohmann::json& json, const codegen_stage_data& value)
    {
        json["files"] = value.files;
        json["steps"] = value.steps;
    }
}
