#pragma once

#include <memory>

#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

#include "spore/codegen/ast/conditions/ast_condition.hpp"
#include "spore/codegen/codegen_error.hpp"

namespace spore::codegen
{
    struct ast_condition_attribute : ast_condition
    {
        nlohmann::json attributes;

        static constexpr std::string_view type()
        {
            return "attribute";
        }

        static std::shared_ptr<ast_condition> make_condition(const nlohmann::json& data)
        {
            std::shared_ptr<ast_condition_attribute> condition = std::make_shared<ast_condition_attribute>();

            if (data.is_object())
            {
                condition->attributes = data;
            }
            else
            {
                SPDLOG_WARN("invalid condition data, expected JSON object, type={} data={}", data.type_name(), data.dump());
                condition->attributes = nlohmann::json::object();
            }

            return condition;
        }

        bool matches_condition(const ast_file& file) const override
        {
            for (auto it_attribute = attributes.begin(); it_attribute != attributes.end(); ++it_attribute)
            {
                if (!matches_attribute(file, it_attribute.key(), it_attribute.value()))
                {
                    return false;
                }
            }

            return true;
        }

        static bool matches_attribute(const ast_file& file, const std::string& key, const nlohmann::json& attribute)
        {
            const auto matches_object = [&](const auto& ast_object) {
                if (ast_object.attributes.is_object() && ast_object.attributes.contains(key))
                {
                    const nlohmann::json& object_attribute = ast_object.attributes[key];
                    if (!attribute.is_array() && object_attribute.is_array())
                    {
                        return object_attribute.contains(attribute);
                    }
                    else
                    {
                        return object_attribute.type() == attribute.type() && object_attribute == attribute;
                    }
                }

                return false;
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