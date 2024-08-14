#pragma once

#include <atomic>
#include <cstdint>
#include <memory>

#include "nlohmann/json.hpp"

#include "spore/codegen/ast/converters/ast_converter.hpp"
#include "spore/codegen/misc/make_unique_id.hpp"

namespace spore::codegen
{
    void to_json(nlohmann::json& json, ast_flags value)
    {
        const auto predicate = [&](ast_flags flags) { return (value & flags) == flags; };
        json["const"] = predicate(ast_flags::const_);
        json["volatile"] = predicate(ast_flags::volatile_);
        json["mutable"] = predicate(ast_flags::mutable_);
        json["static"] = predicate(ast_flags::static_);
        json["virtual"] = predicate(ast_flags::virtual_);
        json["default"] = predicate(ast_flags::default_);
        json["delete"] = predicate(ast_flags::delete_);
        json["public"] = predicate(ast_flags::public_);
        json["private"] = predicate(ast_flags::private_);
        json["protected"] = predicate(ast_flags::protected_);
        json["constexpr"] = predicate(ast_flags::constexpr_);
        json["consteval"] = predicate(ast_flags::consteval_);
        json["lvalue_ref"] = predicate(ast_flags::lvalue_ref);
        json["rvalue_ref"] = predicate(ast_flags::rvalue_ref);
        json["pointer"] = predicate(ast_flags::pointer);
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
        json["is_template_specialization"] = value.is_template_specialization();
        json["template_params"] = value.template_params;
        json["template_specialization_params"] = value.template_specialization_params;
    }

    template <typename ast_object_t>
    void to_json(nlohmann::json& json, const ast_has_attributes<ast_object_t>& value)
    {
        json["attributes"] = value.attributes.is_object() ? value.attributes : nlohmann::json::object();
    }

    template <typename ast_object_t>
    void to_json(nlohmann::json& json, const ast_has_flags<ast_object_t>& value)
    {
        json["flags"] = value.flags;
    }

    void to_json(nlohmann::json& json, const ast_ref& value)
    {
        to_json(json, static_cast<const ast_has_flags<ast_ref>&>(value));

        json["name"] = value.name;
    }

    void to_json(nlohmann::json& json, const ast_template_param& value)
    {
        std::size_t unique_id = make_unique_id<ast_template_param>();

        json["id"] = unique_id;
        json["name"] = value.name.empty() ? fmt::format("_{}", unique_id) : value.name;
        json["type"] = value.type;
        json["default_value"] = value.default_value.value_or("");
        json["has_default_value"] = value.default_value.has_value();
        json["is_variadic"] = value.is_variadic;
    }

    void to_json(nlohmann::json& json, const ast_argument& value)
    {
        to_json(json, static_cast<const ast_has_attributes<ast_argument>&>(value));

        json["id"] = make_unique_id<ast_argument>();
        json["name"] = value.name;
        json["type"] = value.type;
        json["default_value"] = value.default_value.value_or("");
        json["has_default_value"] = value.default_value.has_value();
        json["is_variadic"] = value.is_variadic;
    }

    void to_json(nlohmann::json& json, const ast_function& value)
    {
        to_json(json, static_cast<const ast_has_name<ast_function>&>(value));
        to_json(json, static_cast<const ast_has_attributes<ast_function>&>(value));
        to_json(json, static_cast<const ast_has_template_params<ast_function>&>(value));
        to_json(json, static_cast<const ast_has_flags<ast_function>&>(value));

        json["id"] = make_unique_id<ast_function>();
        json["arguments"] = value.arguments;
        json["return_type"] = value.return_type;
        json["is_variadic"] = value.is_variadic();
    }

    void to_json(nlohmann::json& json, const ast_constructor& value)
    {
        to_json(json, static_cast<const ast_has_attributes<ast_constructor>&>(value));
        to_json(json, static_cast<const ast_has_template_params<ast_constructor>&>(value));
        to_json(json, static_cast<const ast_has_flags<ast_constructor>&>(value));

        json["id"] = make_unique_id<ast_constructor>();
        json["arguments"] = value.arguments;
    }

    void to_json(nlohmann::json& json, const ast_field& value)
    {
        to_json(json, static_cast<const ast_has_attributes<ast_field>&>(value));
        to_json(json, static_cast<const ast_has_flags<ast_field>&>(value));

        json["id"] = make_unique_id<ast_field>();
        json["name"] = value.name;
        json["type"] = value.type;
    }

    void to_json(nlohmann::json& json, ast_class_type value)
    {
        static const std::map<ast_class_type, std::string> value_map {
            {ast_class_type::unknown, "unknown"},
            {ast_class_type::class_, "class"},
            {ast_class_type::struct_, "struct"},
        };

        const auto it = value_map.find(value);
        if (it != value_map.end())
        {
            json = it->second;
        }
    }

    void to_json(nlohmann::json& json, const ast_class& value)
    {
        to_json(json, static_cast<const ast_has_name<ast_class>&>(value));
        to_json(json, static_cast<const ast_has_attributes<ast_class>&>(value));
        to_json(json, static_cast<const ast_has_template_params<ast_class>&>(value));

        json["id"] = make_unique_id<ast_class>();
        json["type"] = value.type;
        json["bases"] = value.bases;
        json["fields"] = value.fields;
        json["functions"] = value.functions;
        json["constructors"] = value.constructors;
        json["nested"] = value.nested;
    }

    void to_json(nlohmann::json& json, const ast_enum_value& value)
    {
        to_json(json, static_cast<const ast_has_attributes<ast_enum_value>&>(value));

        json["id"] = make_unique_id<ast_enum_value>();
        json["name"] = value.name;
        json["value"] = value.value;
    }

    void to_json(nlohmann::json& json, ast_enum_type value)
    {
        static const std::map<ast_enum_type, std::string> value_map {
            {ast_enum_type::none, "none"},
            {ast_enum_type::enum_, "enum"},
            {ast_enum_type::enum_class, "enum class"},
        };

        const auto it = value_map.find(value);
        if (it != value_map.end())
        {
            json = it->second;
        }
    }

    void to_json(nlohmann::json& json, const ast_enum& value)
    {
        to_json(json, static_cast<const ast_has_name<ast_enum>&>(value));
        to_json(json, static_cast<const ast_has_attributes<ast_enum>&>(value));

        json["id"] = make_unique_id<ast_enum>();
        json["type"] = value.type;
        json["base"] = value.base;
        json["values"] = value.values;
        json["nested"] = value.nested;
    }

    void to_json(nlohmann::json& json, const ast_file& value)
    {
        json["id"] = make_unique_id<ast_file>();
        json["path"] = value.path;
        json["classes"] = value.classes;
        json["enums"] = value.enums;
        json["functions"] = value.functions;
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