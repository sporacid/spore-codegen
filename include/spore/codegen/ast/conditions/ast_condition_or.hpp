#pragma once

#include "spore/codegen/ast/conditions/ast_condition_logical.hpp"

namespace spore::codegen
{
    struct ast_condition_or : ast_condition_logical<ast_condition_or>
    {
        static constexpr std::string_view type()
        {
            return "or";
        }

        bool matches_condition(const ast_file& file) const override
        {
            const auto predicate = [&](const std::shared_ptr<ast_condition>& condition) {
                return condition && condition->matches_condition(file);
            };

            return std::any_of(conditions.begin(), conditions.end(), predicate);
        }
    };
}