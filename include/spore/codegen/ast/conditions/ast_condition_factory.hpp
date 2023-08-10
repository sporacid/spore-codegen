#pragma once

#include <memory>

#include "nlohmann/json.hpp"

#include "spore/codegen/ast/conditions/ast_condition.hpp"

namespace spore::codegen
{
    struct ast_condition_params
    {
        std::string type;
        nlohmann::json value;
    };

    void from_json(const nlohmann::json& json, ast_condition_params& value)
    {
        json["type"].get_to(value.type);
        json["value"].get_to(value.value);
    }

    struct ast_condition_factory
    {
        using factory_func_t = std::function<std::shared_ptr<ast_condition>(const nlohmann::json&)>;

        std::map<std::string, factory_func_t> factory_map;

        static ast_condition_factory& instance()
        {
            static ast_condition_factory factory;
            return factory;
        }

        template <typename condition_t>
        void register_condition()
        {
            register_condition(condition_t::type().data(), &condition_t::make_condition);
        }

        void register_condition(const std::string& type, const factory_func_t& factory_func)
        {
            factory_map.emplace(type, factory_func);
        }

        [[nodiscard]] std::shared_ptr<ast_condition> make_condition(const nlohmann::json& json) const
        {
            ast_condition_params params;
            from_json(json, params);
            return make_condition(params);
        }

        [[nodiscard]] std::shared_ptr<ast_condition> make_condition(const ast_condition_params& params) const
        {
            const auto it = factory_map.find(params.type);
            return it != factory_map.end() ? it->second(params.value) : nullptr;
        }
    };
}