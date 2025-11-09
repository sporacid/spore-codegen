#pragma once

#include "nlohmann/json.hpp"

#include "spore/codegen/parsers/codegen_converter.hpp"
#include "spore/codegen/parsers/slang/ast/slang_module.hpp"

namespace spore::codegen
{
    inline void to_json(nlohmann::json& json, const slang_layout_unit& value)
    {
        static const std::map<slang_layout_unit, std::string> value_map {
            {slang_layout_unit::none, "none"},
            {slang_layout_unit::uniform, "uniform"},
            {slang_layout_unit::mixed, "mixed"},
            {slang_layout_unit::descriptor_table_slot, "descriptor_table_slot"},
            {slang_layout_unit::push_constant_buffer, "push_constant_buffer"},
            {slang_layout_unit::varying_input, "varying_input"},
            {slang_layout_unit::varying_output, "varying_output"},
            {slang_layout_unit::specialization_constant, "specialization_constant"},
            {slang_layout_unit::input_attachment_index, "input_attachment_index"},
            {slang_layout_unit::d3d_constant_buffer, "d3d_constant_buffer"},
            {slang_layout_unit::d3d_shader_resource, "d3d_shader_resource"},
            {slang_layout_unit::d3d_unordered_access, "d3d_unordered_access"},
            {slang_layout_unit::d3d_sampler_state, "d3d_sampler_state"},
        };

        const auto it_value = value_map.find(value);

        if (it_value != value_map.end())
        {
            json = it_value->second;
        }
    }

    inline void to_json(nlohmann::json& json, const slang_attribute_arg& value)
    {
        const auto visitor = [&](const auto& arg) { json = arg; };
        std::visit(visitor, value);
    }

    inline void to_json(nlohmann::json& json, const slang_attribute& value)
    {
        json["name"] = value.name;
        json["args"] = nlohmann::json::array();

        for (const slang_attribute_arg& arg : value.args)
        {
            to_json(json["args"].emplace_back(), arg);
        }
    }

    inline void to_json(nlohmann::json& json, const slang_variable_layout& value)
    {
        json["unit"] = value.unit;
        json["offset"] = value.offset;
    }

    template <typename layout_t>
    void to_json(nlohmann::json& json, const slang_target<layout_t>& value)
    {
        json["name"] = value.name;
        json["layouts"] = value.layouts;
    }

    inline void to_json(nlohmann::json& json, const slang_variable& value)
    {
        json["type"] = value.type;
        json["name"] = value.name;
        json["targets"] = value.targets;
        json["attributes"] = value.attributes;
    }

    inline void to_json(nlohmann::json& json, const slang_type_layout& value)
    {
        json["unit"] = value.unit;
        json["size"] = value.size;
        json["stride"] = value.stride;
        json["alignment"] = value.alignment;
    }

    inline void to_json(nlohmann::json& json, const slang_type& value)
    {
        json["name"] = value.name;
        json["fields"] = value.fields;
        json["targets"] = value.targets;
        json["attributes"] = value.attributes;
    }

    inline void to_json(nlohmann::json& json, const slang_entry_point& value)
    {
        json["name"] = value.name;
        json["inputs"] = value.inputs;
        json["attributes"] = value.attributes;

        if (value.output.has_value())
        {
            json["output"] = value.output.value();
        }
    }

    inline void to_json(nlohmann::json& json, const slang_module& value)
    {
        json["path"] = value.path;
        json["types"] = value.types;
        json["variables"] = value.variables;
        json["entry_points"] = value.entry_points;
    }

    struct codegen_converter_slang final : codegen_converter<slang_module>
    {
        bool convert_ast(const slang_module& module, nlohmann::json& json) const override
        {
            json = module;
            return true;
        }
    };
}