#pragma once

#include <memory>

#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

#include "spore/codegen/conditions/codegen_condition.hpp"
#include "spore/codegen/parsers/cpp/ast/cpp_file.hpp"

namespace spore::codegen
{
    struct codegen_condition_attribute final : codegen_condition<cpp_file>
    {
        nlohmann::json attributes;

        static constexpr std::string_view type()
        {
            return "attribute";
        }

        static std::shared_ptr<codegen_condition_attribute> make_condition(const nlohmann::json& data)
        {
            std::shared_ptr<codegen_condition_attribute> condition = std::make_shared<codegen_condition_attribute>();

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

        bool match_ast(const cpp_file& file) const override
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

      private:
        static bool matches_attribute(const cpp_file& file, const std::string& key, const nlohmann::json& attribute)
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

            for (const cpp_class& class_ : file.classes)
            {
                matches = matches || std::any_of(class_.constructors.begin(), class_.constructors.end(), matches_object);
                matches = matches || std::any_of(class_.functions.begin(), class_.functions.end(), matches_object);
                matches = matches || std::any_of(class_.fields.begin(), class_.fields.end(), matches_object);
            }

            return matches;
        }
    };
}