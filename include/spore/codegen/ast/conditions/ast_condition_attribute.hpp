#pragma once

#include <memory>

#include "nlohmann/json.hpp"

#include "spore/codegen/ast/conditions/ast_condition.hpp"

namespace spore::codegen
{
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

            return std::any_of(attributes.begin(), attributes.end(), predicate);
        }

        static bool matches_attribute(const ast_file& file, const std::string attribute)
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