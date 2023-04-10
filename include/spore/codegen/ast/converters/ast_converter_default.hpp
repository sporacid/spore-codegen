#pragma once

#include <atomic>
#include <cstdint>
#include <memory>

#include "nlohmann/json.hpp"

#include "spore/codegen/ast/converters/ast_converter.hpp"

namespace spore::codegen
{
    namespace details
    {
        std::uint32_t make_unique_id()
        {
            static std::atomic<std::uint32_t> seed;
            return ++seed;
        }
    }

    void to_json(nlohmann::json& json, ast_flags value)
    {
        json["const"] = (value & ast_flags::const_) == ast_flags::const_;
        json["volatile"] = (value & ast_flags::volatile_) == ast_flags::volatile_;
        json["mutable"] = (value & ast_flags::mutable_) == ast_flags::mutable_;
        json["static"] = (value & ast_flags::static_) == ast_flags::static_;
        json["public"] = (value & ast_flags::public_) == ast_flags::public_;
        json["private"] = (value & ast_flags::private_) == ast_flags::private_;
        json["protected"] = (value & ast_flags::protected_) == ast_flags::protected_;
        json["lvalue_ref"] = (value & ast_flags::lvalue_ref) == ast_flags::lvalue_ref;
        json["rvalue_ref"] = (value & ast_flags::rvalue_ref) == ast_flags::rvalue_ref;
        json["pointer"] = (value & ast_flags::pointer) == ast_flags::pointer;
        json["ref"] = (value & (ast_flags::lvalue_ref | ast_flags::rvalue_ref)) != ast_flags::none;
        json["value"] = (value & (ast_flags::lvalue_ref | ast_flags::rvalue_ref)) == ast_flags::none;
    }

    template <typename ast_object_t>
    void to_json(nlohmann::json& json, const ast_has_name<ast_object_t>& value)
    {
        json["name"] = value.name;
        json["scope"] = value.scope;
        json["full_name"] = value.full_name();
    }

    template <typename ast_object_t>
    void to_json(nlohmann::json& json, const ast_has_template_params<ast_object_t>& value)
    {
        json["is_template"] = value.is_template();
        json["template_params"] = value.template_params;
    }

    template <typename ast_object_t>
    void to_json(nlohmann::json& json, const ast_has_attributes<ast_object_t>& value)
    {
        json["attributes"] = value.attributes;
    }

    void to_json(nlohmann::json& json, const ast_attribute& value)
    {
        to_json(json, static_cast<const ast_has_name<ast_attribute>&>(value));

        json["id"] = details::make_unique_id();
        json["values"] = value.values;
    }

    void to_json(nlohmann::json& json, const std::vector<ast_attribute>& value)
    {
        json = nlohmann::json(nlohmann::json::value_t::object);

        for (const ast_attribute& attribute : value)
        {
            json[attribute.full_name()] = attribute;
        }
    }

    void to_json(nlohmann::json& json, const ast_ref& value)
    {
        json["id"] = details::make_unique_id();
        json["name"] = value.name;
        json["flags"] = value.flags;
    }

    void to_json(nlohmann::json& json, const ast_template_param& value)
    {
        json["id"] = details::make_unique_id();
        json["name"] = value.name;
        json["type"] = value.type;
        json["default_value"] = value.default_value.value_or("");
        json["has_default_value"] = value.default_value.has_value();
    }

    void to_json(nlohmann::json& json, const ast_argument& value)
    {
        to_json(json, static_cast<const ast_has_attributes<ast_argument>&>(value));

        json["id"] = details::make_unique_id();
        json["name"] = value.name;
        json["type"] = value.type;
        json["default_value"] = value.default_value.value_or("");
        json["has_default_value"] = value.default_value.has_value();
    }

    void to_json(nlohmann::json& json, const ast_function& value)
    {
        to_json(json, static_cast<const ast_has_name<ast_function>&>(value));
        to_json(json, static_cast<const ast_has_attributes<ast_function>&>(value));
        to_json(json, static_cast<const ast_has_template_params<ast_function>&>(value));

        json["id"] = details::make_unique_id();
        json["flags"] = value.flags;
        json["arguments"] = value.arguments;
        json["return_type"] = value.return_type;
    }

    void to_json(nlohmann::json& json, const ast_field& value)
    {
        to_json(json, static_cast<const ast_has_attributes<ast_field>&>(value));

        json["id"] = details::make_unique_id();
        json["name"] = value.name;
        json["type"] = value.type;
    }

    void to_json(nlohmann::json& json, const ast_class& value)
    {
        to_json(json, static_cast<const ast_has_name<ast_class>&>(value));
        to_json(json, static_cast<const ast_has_attributes<ast_class>&>(value));
        to_json(json, static_cast<const ast_has_template_params<ast_class>&>(value));

        json["id"] = details::make_unique_id();
        json["type"] = value.type;
        json["bases"] = value.bases;
        json["fields"] = value.fields;
        json["functions"] = value.functions;
    }

    void to_json(nlohmann::json& json, const ast_enum_value& value)
    {
        to_json(json, static_cast<const ast_has_attributes<ast_enum_value>&>(value));

        json["name"] = value.name;
        json["value"] = value.value.value_or("");
        json["has_value"] = value.value.has_value();
    }

    void to_json(nlohmann::json& json, const ast_enum& value)
    {
        to_json(json, static_cast<const ast_has_name<ast_enum>&>(value));
        to_json(json, static_cast<const ast_has_attributes<ast_enum>&>(value));
        json["id"] = details::make_unique_id();
        json["enum_values"] = value.enum_values;
    }

    void to_json(nlohmann::json& json, const ast_include& value)
    {
        json["name"] = value.name;
        json["path"] = value.path;
    }

    void to_json(nlohmann::json& json, const ast_file& value)
    {
        json["id"] = details::make_unique_id();
        json["path"] = value.path;
        json["classes"] = value.classes;
        json["enums"] = value.enums;
        json["functions"] = value.functions;
        json["includes"] = value.includes;
    }

    struct ast_converter_default : ast_converter
    {
        bool convert_file(const ast_file& file, nlohmann::json& json) const override
        {
            json = file;
            return true;
        }
    };
}