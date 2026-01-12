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

    inline void to_json(nlohmann::json& json, const slang_stage& value)
    {
        static const std::map<slang_stage, std::string> value_map {
            {slang_stage::none, "none"},
            {slang_stage::vertex, "vertex"},
            {slang_stage::hull, "hull"},
            {slang_stage::domain, "domain"},
            {slang_stage::geometry, "geometry"},
            {slang_stage::fragment, "fragment"},
            {slang_stage::compute, "compute"},
            {slang_stage::ray_generation, "ray_generation"},
            {slang_stage::ray_intersection, "ray_intersection"},
            {slang_stage::ray_any_hit, "ray_any_hit"},
            {slang_stage::ray_closest_hit, "ray_closest_hit"},
            {slang_stage::ray_miss, "ray_miss"},
            {slang_stage::callable, "callable"},
            {slang_stage::mesh, "mesh"},
            {slang_stage::mesh_amplification, "mesh_amplification"},
            {slang_stage::mesh_dispatch, "mesh_dispatch"},
        };

        const auto it_value = value_map.find(value);

        if (it_value != value_map.end())
        {
            json = it_value->second;
        }
#if 0
        const auto predicate = [&](const slang_stage stages) { return (value & stages) == stages; };

        json["vertex"] = predicate(slang_stage::vertex);
        json["hull"] = predicate(slang_stage::hull);
        json["domain"] = predicate(slang_stage::domain);
        json["geometry"] = predicate(slang_stage::geometry);
        json["fragment"] = predicate(slang_stage::fragment);
        json["compute"] = predicate(slang_stage::compute);
        json["ray_generation"] = predicate(slang_stage::ray_generation);
        json["ray_intersection"] = predicate(slang_stage::ray_intersection);
        json["ray_any_hit"] = predicate(slang_stage::ray_any_hit);
        json["ray_closest_hit"] = predicate(slang_stage::ray_closest_hit);
        json["ray_miss"] = predicate(slang_stage::ray_miss);
        json["callable"] = predicate(slang_stage::callable);
        json["mesh"] = predicate(slang_stage::mesh);
        json["mesh_amplification"] = predicate(slang_stage::mesh_amplification);
        json["mesh_dispatch"] = predicate(slang_stage::mesh_dispatch);
#endif
    }

    inline void to_json(nlohmann::json& json, const slang_image_format& value)
    {
        static const std::map<slang_image_format, std::string> value_map {
            {slang_image_format::none, "none"},
            {slang_image_format::unknown, "unknown"},
            {slang_image_format::rgba32f, "rgba32f"},
            {slang_image_format::rgba16f, "rgba16f"},
            {slang_image_format::rg32f, "rg32f"},
            {slang_image_format::rg16f, "rg16f"},
            {slang_image_format::r11f_g11f_b10f, "r11f_g11f_b10f"},
            {slang_image_format::r32f, "r32f"},
            {slang_image_format::r16f, "r16f"},
            {slang_image_format::rgba16, "rgba16"},
            {slang_image_format::rgb10_a2, "rgb10_a2"},
            {slang_image_format::rgba8, "rgba8"},
            {slang_image_format::rg16, "rg16"},
            {slang_image_format::rg8, "rg8"},
            {slang_image_format::r16, "r16"},
            {slang_image_format::r8, "r8"},
            {slang_image_format::rgba16_snorm, "rgba16_snorm"},
            {slang_image_format::rgba8_snorm, "rgba8_snorm"},
            {slang_image_format::rg16_snorm, "rg16_snorm"},
            {slang_image_format::rg8_snorm, "rg8_snorm"},
            {slang_image_format::r16_snorm, "r16_snorm"},
            {slang_image_format::r8_snorm, "r8_snorm"},
            {slang_image_format::rgba32i, "rgba32i"},
            {slang_image_format::rgba16i, "rgba16i"},
            {slang_image_format::rgba8i, "rgba8i"},
            {slang_image_format::rg32i, "rg32i"},
            {slang_image_format::rg16i, "rg16i"},
            {slang_image_format::rg8i, "rg8i"},
            {slang_image_format::r32i, "r32i"},
            {slang_image_format::r16i, "r16i"},
            {slang_image_format::r8i, "r8i"},
            {slang_image_format::rgba32ui, "rgba32ui"},
            {slang_image_format::rgba16ui, "rgba16ui"},
            {slang_image_format::rgb10_a2ui, "rgb10_a2ui"},
            {slang_image_format::rgba8ui, "rgba8ui"},
            {slang_image_format::rg32ui, "rg32ui"},
            {slang_image_format::rg16ui, "rg16ui"},
            {slang_image_format::rg8ui, "rg8ui"},
            {slang_image_format::r32ui, "r32ui"},
            {slang_image_format::r16ui, "r16ui"},
            {slang_image_format::r8ui, "r8ui"},
            {slang_image_format::r64ui, "r64ui"},
            {slang_image_format::r64i, "r64i"},
            {slang_image_format::bgra8, "bgra8"},
        };

        const auto it_value = value_map.find(value);

        if (it_value != value_map.end())
        {
            json = it_value->second;
        }
    }

    inline void to_json(nlohmann::json& json, const slang_variable_layout& value)
    {
        json["unit"] = value.unit;
        json["offset"] = value.offset;
        json["stage"] = value.stage;
        json["image_format"] = value.image_format;
    }

    template <typename layout_t>
    void to_json(nlohmann::json& json, const slang_target<layout_t>& value)
    {
        json["name"] = value.name;
        json["layouts"] = value.layouts;
    }

    inline void to_json(nlohmann::json& json, const slang_modifier& value)
    {
        const auto predicate = [&](const slang_modifier modifiers) { return (value & modifiers) == modifiers; };

        json["shared"] = predicate(slang_modifier::shared);
        json["no_diff"] = predicate(slang_modifier::no_diff);
        json["static"] = predicate(slang_modifier::static_);
        json["const"] = predicate(slang_modifier::const_);
        json["export"] = predicate(slang_modifier::export_);
        json["extern"] = predicate(slang_modifier::extern_);
        json["differentiable"] = predicate(slang_modifier::differentiable);
        json["mutating"] = predicate(slang_modifier::mutating);
        json["in"] = predicate(slang_modifier::in);
        json["out"] = predicate(slang_modifier::out);
        json["in_out"] = predicate(slang_modifier::in_out);
    }

    inline void to_json(nlohmann::json& json, const slang_variable& value)
    {
        json["type"] = value.type;
        json["name"] = value.name;
        json["targets"] = value.targets;
        json["attributes"] = value.attributes;
        json["modifiers"] = value.modifiers;
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
        json["arguments"] = value.arguments;
        json["stage"] = value.stage;
        json["modifiers"] = value.modifiers;
        json["attributes"] = value.attributes;

        if (value.return_type.has_value())
        {
            json["return_type"] = value.return_type.value();
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