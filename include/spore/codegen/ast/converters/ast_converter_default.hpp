#pragma once

#include <atomic>
#include <cstdint>
#include <memory>

#include "nlohmann/json.hpp"

#include "spore/codegen/ast/converters/ast_converter.hpp"

namespace spore::codegen
{
    namespace detail
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
        json["virtual"] = (value & ast_flags::static_) == ast_flags::virtual_;
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

        json["id"] = detail::make_unique_id();
        json["name"] = value.name;
    }

    void to_json(nlohmann::json& json, const ast_template_param& value)
    {
        std::uint32_t unique_id = detail::make_unique_id();

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

        json["id"] = detail::make_unique_id();
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
        to_json(json, static_cast<const ast_has_flags<ast_function>&>(value));

        json["id"] = detail::make_unique_id();
        json["arguments"] = value.arguments;
        json["return_type"] = value.return_type;
    }

    void to_json(nlohmann::json& json, const ast_constructor& value)
    {
        to_json(json, static_cast<const ast_has_attributes<ast_constructor>&>(value));
        to_json(json, static_cast<const ast_has_template_params<ast_constructor>&>(value));
        to_json(json, static_cast<const ast_has_flags<ast_constructor>&>(value));

        json["id"] = detail::make_unique_id();
        json["arguments"] = value.arguments;
    }

    void to_json(nlohmann::json& json, const ast_field& value)
    {
        to_json(json, static_cast<const ast_has_attributes<ast_field>&>(value));
        to_json(json, static_cast<const ast_has_flags<ast_field>&>(value));

        json["id"] = detail::make_unique_id();
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

        json["id"] = detail::make_unique_id();
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

        json["id"] = detail::make_unique_id();
        json["type"] = value.type;
        json["base"] = value.base;
        json["values"] = value.values;
        json["nested"] = value.nested;
    }

    void to_json(nlohmann::json& json, const ast_file& value)
    {
        json["id"] = detail::make_unique_id();
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