#pragma once

#include "base64/base64.hpp"
#include "nlohmann/json.hpp"

#include "spore/codegen/misc/make_unique_id.hpp"
#include "spore/codegen/parsers/codegen_converter.hpp"
#include "spore/codegen/parsers/spirv/ast/spirv_module.hpp"

namespace spore::codegen
{
    inline void to_json(nlohmann::json& json, const spirv_module_stage& value)
    {
        static const std::map<spirv_module_stage, std::string_view> value_map {
            {spirv_module_stage::none, "none"},
            {spirv_module_stage::vertex, "vertex"},
            {spirv_module_stage::fragment, "fragment"},
            {spirv_module_stage::compute, "compute"},
            {spirv_module_stage::geometry, "geometry"},
            {spirv_module_stage::mesh, "mesh"},
            {spirv_module_stage::tesselation_control, "tesselation_control"},
            {spirv_module_stage::tesselation_eval, "tesselation_eval"},
        };

        const auto it_value = value_map.find(value);
        if (it_value != value_map.end())
        {
            json = it_value->second.data();
        }
    }

    inline void to_json(nlohmann::json& json, const spirv_descriptor_kind& value)
    {
        static const std::map<spirv_descriptor_kind, std::string_view> value_map {
            {spirv_descriptor_kind::none, "none"},
            {spirv_descriptor_kind::uniform, "uniform"},
            {spirv_descriptor_kind::sampler, "sampler"},
            {spirv_descriptor_kind::storage, "storage"},
        };

        const auto it_value = value_map.find(value);
        if (it_value != value_map.end())
        {
            json = it_value->second.data();
        }
    }

    inline void to_json(nlohmann::json& json, const spirv_variable_kind& value)
    {
        static const std::map<spirv_variable_kind, std::string_view> value_map {
            {spirv_variable_kind::none, "none"},
            {spirv_variable_kind::input, "input"},
            {spirv_variable_kind::output, "output"},
        };

        const auto it_value = value_map.find(value);
        if (it_value != value_map.end())
        {
            json = it_value->second.data();
        }
    }

    inline void to_json(nlohmann::json& json, const spirv_variable& value)
    {
        json["id"] = make_unique_id<spirv_variable>();
        json["kind"] = value.kind;
        json["name"] = value.name;
        json["location"] = value.location;
        json["size"] = value.size;
        json["count"] = value.count;
        json["format"] = value.format;
    }

    inline void to_json(nlohmann::json& json, const spirv_constant& value)
    {
        json["id"] = make_unique_id<spirv_constant>();
        json["name"] = value.name;
        json["offset"] = value.offset;
        json["variables"] = value.variables;
    }

    inline void to_json(nlohmann::json& json, const spirv_descriptor_binding& value)
    {
        json["id"] = make_unique_id<spirv_descriptor_binding>();
        json["kind"] = value.kind;
        json["name"] = value.name;
        json["index"] = value.index;
        json["count"] = value.count;
        json["variables"] = value.variables;
    }

    inline void to_json(nlohmann::json& json, const spirv_descriptor_set& value)
    {
        json["id"] = make_unique_id<spirv_descriptor_set>();
        json["index"] = value.index;
        json["bindings"] = value.bindings;
    }

    inline void to_json(nlohmann::json& json, const spirv_module& value)
    {
        json["id"] = make_unique_id<spirv_module>();
        json["path"] = value.path;
        json["entry_point"] = value.entry_point;
        json["inputs"] = value.inputs;
        json["outputs"] = value.outputs;
        json["constants"] = value.constants;
        json["descriptor_sets"] = value.descriptor_sets;

        json["byte_code"] = base64::encode_into<std::string>(value.byte_code.begin(), value.byte_code.end());
    }

    struct codegen_converter_spirv final : codegen_converter<spirv_module>
    {
        bool convert_ast(const spirv_module& module, nlohmann::json& json) const override
        {
            json = module;
            return true;
        }
    };
}