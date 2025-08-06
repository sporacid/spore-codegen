#pragma once

#include <cstdint>
#include <format>

#include "nlohmann/json.hpp"

#include "spore/codegen/misc/make_unique_id.hpp"
#include "spore/codegen/parsers/codegen_converter.hpp"
#include "spore/codegen/parsers/cpp/ast/cpp_file.hpp"

namespace spore::codegen
{
    inline void to_json(nlohmann::json& json, cpp_flags value)
    {
        const auto predicate = [&](cpp_flags flags) { return (value & flags) == flags; };
        json["const"] = predicate(cpp_flags::const_);
        json["volatile"] = predicate(cpp_flags::volatile_);
        json["mutable"] = predicate(cpp_flags::mutable_);
        json["static"] = predicate(cpp_flags::static_);
        json["virtual"] = predicate(cpp_flags::virtual_);
        json["default"] = predicate(cpp_flags::default_);
        json["delete"] = predicate(cpp_flags::delete_);
        json["public"] = predicate(cpp_flags::public_);
        json["private"] = predicate(cpp_flags::private_);
        json["protected"] = predicate(cpp_flags::protected_);
        json["constexpr"] = predicate(cpp_flags::constexpr_);
        json["consteval"] = predicate(cpp_flags::consteval_);
        json["lvalue_ref"] = predicate(cpp_flags::lvalue_ref);
        json["rvalue_ref"] = predicate(cpp_flags::rvalue_ref);
        json["pointer"] = predicate(cpp_flags::pointer);
        json["ref"] = (value & (cpp_flags::lvalue_ref | cpp_flags::rvalue_ref)) != cpp_flags::none;
        json["value"] = (value & (cpp_flags::lvalue_ref | cpp_flags::rvalue_ref)) == cpp_flags::none;
    }

    template <typename cpp_object_t>
    void to_json(nlohmann::json& json, const cpp_has_name<cpp_object_t>& value)
    {
        json["name"] = value.name;
        json["scope"] = value.scope;
        json["full_name"] = value.full_name();
    }

    template <typename cpp_object_t>
    void to_json(nlohmann::json& json, const cpp_has_template_params<cpp_object_t>& value)
    {
        json["is_template"] = value.is_template();
        json["is_template_specialization"] = value.is_template_specialization();
        json["template_params"] = value.template_params;
        json["template_specialization_params"] = value.template_specialization_params;
    }

    template <typename cpp_object_t>
    void to_json(nlohmann::json& json, const cpp_has_attributes<cpp_object_t>& value)
    {
        json["attributes"] = value.attributes.is_object() ? value.attributes : nlohmann::json::object();
    }

    template <typename cpp_object_t>
    void to_json(nlohmann::json& json, const cpp_has_flags<cpp_object_t>& value)
    {
        json["flags"] = value.flags;
    }

    inline void to_json(nlohmann::json& json, const cpp_ref& value)
    {
        to_json(json, static_cast<const cpp_has_flags<cpp_ref>&>(value));

        json["name"] = value.name;
    }

    inline void to_json(nlohmann::json& json, const cpp_template_param& value)
    {
        std::size_t unique_id = make_unique_id<cpp_template_param>();

        json["id"] = unique_id;
        json["name"] = value.name.empty() ? std::format("_{}", unique_id) : value.name;
        json["type"] = value.type;
        json["default_value"] = value.default_value.value_or("");
        json["has_default_value"] = value.default_value.has_value();
        json["is_variadic"] = value.is_variadic;
    }

    inline void to_json(nlohmann::json& json, const cpp_argument& value)
    {
        to_json(json, static_cast<const cpp_has_attributes<cpp_argument>&>(value));

        json["id"] = make_unique_id<cpp_argument>();
        json["name"] = value.name;
        json["type"] = value.type;
        json["default_value"] = value.default_value.value_or("");
        json["has_default_value"] = value.default_value.has_value();
        json["is_variadic"] = value.is_variadic;
    }

    inline void to_json(nlohmann::json& json, const cpp_function& value)
    {
        to_json(json, static_cast<const cpp_has_name<cpp_function>&>(value));
        to_json(json, static_cast<const cpp_has_attributes<cpp_function>&>(value));
        to_json(json, static_cast<const cpp_has_template_params<cpp_function>&>(value));
        to_json(json, static_cast<const cpp_has_flags<cpp_function>&>(value));

        json["id"] = make_unique_id<cpp_function>();
        json["arguments"] = value.arguments;
        json["return_type"] = value.return_type;
        json["is_variadic"] = value.is_variadic();
    }

    inline void to_json(nlohmann::json& json, const cpp_constructor& value)
    {
        to_json(json, static_cast<const cpp_has_attributes<cpp_constructor>&>(value));
        to_json(json, static_cast<const cpp_has_template_params<cpp_constructor>&>(value));
        to_json(json, static_cast<const cpp_has_flags<cpp_constructor>&>(value));

        json["id"] = make_unique_id<cpp_constructor>();
        json["arguments"] = value.arguments;
    }

    inline void to_json(nlohmann::json& json, const cpp_field& value)
    {
        to_json(json, static_cast<const cpp_has_attributes<cpp_field>&>(value));
        to_json(json, static_cast<const cpp_has_flags<cpp_field>&>(value));

        json["id"] = make_unique_id<cpp_field>();
        json["name"] = value.name;
        json["type"] = value.type;
    }

    inline void to_json(nlohmann::json& json, cpp_class_type value)
    {
        static const std::map<cpp_class_type, std::string> value_map {
            {cpp_class_type::unknown, "unknown"},
            {cpp_class_type::class_, "class"},
            {cpp_class_type::struct_, "struct"},
        };

        const auto it = value_map.find(value);
        if (it != value_map.end())
        {
            json = it->second;
        }
    }

    inline void to_json(nlohmann::json& json, const cpp_class& value)
    {
        to_json(json, static_cast<const cpp_has_name<cpp_class>&>(value));
        to_json(json, static_cast<const cpp_has_attributes<cpp_class>&>(value));
        to_json(json, static_cast<const cpp_has_template_params<cpp_class>&>(value));

        json["id"] = make_unique_id<cpp_class>();
        json["type"] = value.type;
        json["bases"] = value.bases;
        json["fields"] = value.fields;
        json["functions"] = value.functions;
        json["constructors"] = value.constructors;
        json["nested"] = value.nested;
        json["definition"] = value.definition;
    }

    inline void to_json(nlohmann::json& json, const cpp_enum_value& value)
    {
        to_json(json, static_cast<const cpp_has_attributes<cpp_enum_value>&>(value));

        json["id"] = make_unique_id<cpp_enum_value>();
        json["name"] = value.name;
        json["value"] = value.value;
    }

    inline void to_json(nlohmann::json& json, const cpp_enum_type value)
    {
        static const std::map<cpp_enum_type, std::string> value_map {
            {cpp_enum_type::none, "none"},
            {cpp_enum_type::enum_, "enum"},
            {cpp_enum_type::enum_class, "enum class"},
        };

        const auto it = value_map.find(value);
        if (it != value_map.end())
        {
            json = it->second;
        }
    }

    inline void to_json(nlohmann::json& json, const cpp_enum& value)
    {
        to_json(json, static_cast<const cpp_has_name<cpp_enum>&>(value));
        to_json(json, static_cast<const cpp_has_attributes<cpp_enum>&>(value));

        json["id"] = make_unique_id<cpp_enum>();
        json["type"] = value.type;
        json["base"] = value.base;
        json["values"] = value.values;
        json["nested"] = value.nested;
        json["definition"] = value.definition;
    }

    inline void to_json(nlohmann::json& json, const cpp_file& value)
    {
        json["id"] = make_unique_id<cpp_file>();
        json["path"] = value.path;
        json["classes"] = value.classes;
        json["enums"] = value.enums;
        json["functions"] = value.functions;
    }

    struct codegen_converter_cpp final : codegen_converter<cpp_file>
    {
        bool convert_ast(const cpp_file& file, nlohmann::json& json) const override
        {
            json = file;
            return true;
        }
    };
}