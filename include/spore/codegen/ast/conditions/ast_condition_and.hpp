#pragma once

#include "spore/codegen/ast/conditions/ast_condition_logical.hpp"

namespace spore::codegen
{
    struct ast_condition_and : ast_condition_logical<ast_condition_and>
    {
        static constexpr std::string_view type()
        {
            return "and";
        }

        bool matches_condition(const ast_file& file) const override
        {
            const auto predicate = [&](const std::shared_ptr<ast_condition>& condition) {
                return condition && condition->matches_condition(file);
            };

            return std::all_of(conditions.begin(), conditions.end(), predicate);
        }
    };
}