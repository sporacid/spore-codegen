#pragma once

#include <memory>

#include "nlohmann/json.hpp"

#include "spore/codegen/ast/conditions/ast_condition.hpp"

namespace spore::codegen
{
    //    enum class ast_condition_attribute_match
    //    {
    //        any,
    //        all,
    //        none,
    //    };
    //
    //    void from_json(const nlohmann::json& json, ast_condition_attribute_match& value)
    //    {
    //        static std::map<std::string, ast_condition_attribute_match> value_map {
    //            {"any", ast_condition_attribute_match::any},
    //            {"all", ast_condition_attribute_match::all},
    //            {"none", ast_condition_attribute_match::none},
    //        };
    //
    //        const auto it = value_map.find(json.get<std::string>());
    //        value = it != value_map.end() ? it->second : ast_condition_attribute_match::any;
    //    }
    //
    //    struct ast_condition_attribute_params
    //    {
    //        ast_condition_attribute_match match = ast_condition_attribute_match::any;
    //        std::vector<std::string> attributes;
    //    };
    //
    //    void from_json(const nlohmann::json& json, ast_condition_attribute_params& value)
    //    {
    //        json["attributes"].get_to(value.attributes);
    //
    //        if (json["match"])
    //        {
    //            json["match"].get_to(value.match);
    //        }
    //    }

    //    struct ast_condition_attribute : ast_condition
    //    {
    //        // ast_condition_attribute_params params;
    //        std::vector<std::string> attributes;
    //
    //        static constexpr std::string_view type()
    //        {
    //            return "attribute";
    //        }
    //
    //        static std::shared_ptr<ast_condition> make_condition(const nlohmann::json& data)
    //        {
    //            std::shared_ptr<ast_condition_attribute> condition = std::make_shared<ast_condition_attribute>();
    //            condition->params = data;
    //            return condition;
    //        }
    //
    //        bool matches_condition(const ast_file& file) const override
    //        {
    //            const auto predicate = [&](const std::string& attribute) {
    //                return matches_attribute(file, attribute);
    //            };
    //
    //            switch (params.match)
    //            {
    //                case ast_condition_attribute_match::any:
    //                    return std::any_of(params.attributes.begin(), params.attributes.end(), predicate);
    //                case ast_condition_attribute_match::all:
    //                    return std::all_of(params.attributes.begin(), params.attributes.end(), predicate);
    //                case ast_condition_attribute_match::none:
    //                    return std::none_of(params.attributes.begin(), params.attributes.end(), predicate);
    //                default:
    //                    return false;
    //            }
    //        }
    //
    //        bool matches_attribute(const ast_file& file, const std::string attribute) const
    //        {
    //            const auto matches_attribute = [&](const ast_attribute& ast_attribute) {
    //                return attribute == ast_attribute.full_name();
    //            };
    //
    //            const auto matches_object = [&](const auto& ast_object) {
    //                return std::any_of(ast_object.attributes.begin(), ast_object.attributes.end(), matches_attribute);
    //            };
    //
    //            bool matches = std::any_of(file.classes.begin(), file.classes.end(), matches_object);
    //            matches = matches || std::any_of(file.enums.begin(), file.enums.end(), matches_object);
    //            matches = matches || std::any_of(file.functions.begin(), file.functions.end(), matches_object);
    //
    //            for (const ast_class& ast_class : file.classes)
    //            {
    //                matches = matches || std::any_of(ast_class.functions.begin(), ast_class.functions.end(), matches_object);
    //                matches = matches || std::any_of(ast_class.fields.begin(), ast_class.fields.end(), matches_object);
    //            }
    //
    //            return matches;
    //        }
    //    };
    struct ast_condition_attribute : ast_condition
    {
        std::vector<std::string> attributes;

        static constexpr std::string_view type()
        {
            return "attribute";
        }

        static std::shared_ptr<ast_condition> make_condition(const nlohmann::json& data)
        {
            std::vector<std::string> attributes;
            if (data.is_string())
            {
                attributes.emplace_back() = data.get<std::string>();
            }
            else
            {
                attributes = data;
            }

            std::shared_ptr<ast_condition_attribute> condition = std::make_shared<ast_condition_attribute>();
            condition->attributes = std::move(attributes);
            return condition;
        }

        bool matches_condition(const ast_file& file) const override
        {
            const auto predicate = [&](const std::string& attribute) {
                return matches_attribute(file, attribute);
            };

            return std::all_of(attributes.begin(), attributes.end(), predicate);
        }

        bool matches_attribute(const ast_file& file, const std::string attribute) const
        {
            const auto matches_attribute = [&](const ast_attribute& ast_attribute) {
                return attribute == ast_attribute.full_name();
            };

            const auto matches_object = [&](const auto& ast_object) {
                return std::any_of(ast_object.attributes.begin(), ast_object.attributes.end(), matches_attribute);
            };

            bool matches = std::any_of(file.classes.begin(), file.classes.end(), matches_object);
            matches = matches || std::any_of(file.enums.begin(), file.enums.end(), matches_object);
            matches = matches || std::any_of(file.functions.begin(), file.functions.end(), matches_object);

            for (const ast_class& ast_class : file.classes)
            {
                matches = matches || std::any_of(ast_class.functions.begin(), ast_class.functions.end(), matches_object);
                matches = matches || std::any_of(ast_class.fields.begin(), ast_class.fields.end(), matches_object);
            }

            return matches;
        }
    };
}