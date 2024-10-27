#pragma once

#include "base64/base64.hpp"
#include "nlohmann/json.hpp"

#include "spore/codegen/parsers/codegen_converter.hpp"
#include "spore/codegen/parsers/spirv/ast/spirv_module.hpp"

namespace spore::codegen
{
    template <typename... args_t>
    void to_json(nlohmann::json& json, const std::variant<args_t...>& variant)
    {
        const auto visitor = [&](const auto& value) { json = value; };
        std::visit(visitor, variant);
    }

    inline void to_json(nlohmann::json& json, const spirv_module_stage value)
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

    inline void to_json(nlohmann::json& json, const spirv_descriptor_kind value)
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

    inline void to_json(nlohmann::json& json, const spirv_scalar_kind value)
    {
        static const std::map<spirv_scalar_kind, std::string_view> name_map {
            {spirv_scalar_kind::none, "void"},
            {spirv_scalar_kind::bool_, "bool"},
            {spirv_scalar_kind::int_, "int"},
            {spirv_scalar_kind::float_, "float"},
        };

        const auto it_name = name_map.find(value);

        if (it_name != name_map.end())
        {
            json = it_name->second;
        }
    }

    inline void to_json(nlohmann::json& json, const spirv_scalar& value)
    {
        json["kind"] = value.kind;
        json["width"] = value.width;
        json["signed"] = value.signed_;
        json["is_scalar"] = true;
    }

    inline void to_json(nlohmann::json& json, const spirv_vec& value)
    {
        json["scalar"] = value.scalar;
        json["components"] = value.components;
        json["is_vec"] = true;
    }

    inline void to_json(nlohmann::json& json, const spirv_mat& value)
    {
        json["scalar"] = value.scalar;
        json["rows"] = value.rows;
        json["cols"] = value.cols;
        json["row_major"] = value.row_major;
        json["is_mat"] = true;
    }

    inline void to_json(nlohmann::json& json, const spirv_struct& value)
    {
        json["name"] = value.name;
        json["is_struct"] = true;
    }

    inline void to_json(nlohmann::json& json, const spirv_image& value)
    {
        json["dims"] = value.dims;
        json["depth"] = value.depth;
        json["sampled"] = value.sampled;
        json["is_image"] = true;
    }

    inline void to_json(nlohmann::json& json, const spirv_builtin& value)
    {
        json["name"] = value.name;
        json["is_builtin"] = true;
    }

    inline void to_json(nlohmann::json& json, const spirv_array& value)
    {
        json["type"] = value.type;
        json["dims"] = value.dims;
        json["is_array"] = true;
    }

    inline void to_json(nlohmann::json& json, const spirv_variable& value)
    {
        json["name"] = value.name;
        json["index"] = value.index;
        json["type"] = value.type;
    }

    inline void to_json(nlohmann::json& json, const spirv_constant& value)
    {
        json["name"] = value.name;
        json["type"] = value.type;
        json["offset"] = value.offset;
        json["variables"] = value.variables;
    }

    inline void to_json(nlohmann::json& json, const spirv_descriptor_binding& value)
    {
        json["kind"] = value.kind;
        json["name"] = value.name;
        json["type"] = value.type;
        json["index"] = value.index;
        json["count"] = value.count;
        json["variables"] = value.variables;
    }

    inline void to_json(nlohmann::json& json, const spirv_descriptor_set& value)
    {
        json["index"] = value.index;
        json["bindings"] = value.bindings;
    }

    inline void to_json(nlohmann::json& json, const spirv_module& value)
    {
        json["path"] = value.path;
        json["stage"] = value.stage;
        json["entry_point"] = value.entry_point;
        json["inputs"] = value.inputs;
        json["outputs"] = value.outputs;
        json["constants"] = value.constants;
        json["descriptor_sets"] = value.descriptor_sets;
        json["byte_code_size"] = value.byte_code.size();
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