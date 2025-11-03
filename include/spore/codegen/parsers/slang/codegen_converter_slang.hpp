#pragma once

#include "nlohmann/json.hpp"

#include "spore/codegen/parsers/codegen_converter.hpp"
#include "spore/codegen/parsers/slang/ast/slang_module.hpp"

namespace spore::codegen
{
    inline void to_json(nlohmann::json& json, const slang_attribute& value)
    {
        json["type"] = value.type;
        json["values"] = value.values;
    }

    inline void to_json(nlohmann::json& json, const slang_variable& value)
    {
        json["type"] = value.type;
        json["name"] = value.name;
        json["attributes"] = value.attributes;
    }

    inline void to_json(nlohmann::json& json, const slang_struct& value)
    {
        json["name"] = value.name;
        json["fields"] = value.fields;
        json["attributes"] = value.attributes;
    }

    inline void to_json(nlohmann::json& json, const slang_entry_point& value)
    {
        json["name"] = value.name;
        json["attributes"] = value.attributes;
    }

    inline void to_json(nlohmann::json& json, const slang_module& value)
    {
        json["path"] = value.path;
        json["structs"] = value.structs;
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