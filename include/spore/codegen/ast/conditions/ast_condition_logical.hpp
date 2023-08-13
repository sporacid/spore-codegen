#pragma once

#include "spore/codegen/ast/conditions/ast_condition.hpp"
#include "spore/codegen/ast/conditions/ast_condition_factory.hpp"

namespace spore::codegen
{
    template <typename condition_t>
    struct ast_condition_logical : ast_condition
    {
        std::vector<std::shared_ptr<ast_condition>> conditions;

        static std::shared_ptr<ast_condition> make_condition(const nlohmann::json& data)
        {
            std::vector<nlohmann::json> params = data.get<std::vector<nlohmann::json>>();

            const auto make_inner_condition = [](const nlohmann::json& json) {
                return ast_condition_factory::instance().make_condition(json);
            };

            std::vector<std::shared_ptr<ast_condition>> conditions;
            conditions.reserve(params.size());
            std::transform(params.begin(), params.end(), std::back_inserter(conditions), make_inner_condition);

            std::shared_ptr<condition_t> condition = std::make_shared<condition_t>();
            condition->conditions = std::move(conditions);
            return condition;
        }
    };
}