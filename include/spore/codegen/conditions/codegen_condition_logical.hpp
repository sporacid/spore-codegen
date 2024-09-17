#pragma once

#include <memory>
#include <string_view>

#include "nlohmann/json.hpp"

#include "spore/codegen/conditions/codegen_condition.hpp"
#include "spore/codegen/conditions/codegen_condition_factory.hpp"

namespace spore::codegen
{
    template <typename ast_t, typename condition_t>
    struct codegen_condition_logical : codegen_condition<ast_t>
    {
        using condition_ptr = std::shared_ptr<codegen_condition<ast_t>>;
        std::vector<condition_ptr> conditions;

        static std::shared_ptr<condition_t> make_condition(const nlohmann::json& data)
        {
            std::vector<nlohmann::json> params = data.get<std::vector<nlohmann::json>>();
            const auto make_inner_condition = [](const nlohmann::json& json) {
                return codegen_condition_factory<ast_t>::get().make_condition(json);
            };

            std::vector<condition_ptr> conditions;
            conditions.reserve(params.size());
            std::transform(params.begin(), params.end(), std::back_inserter(conditions), make_inner_condition);

            std::shared_ptr<condition_t> condition = std::make_shared<condition_t>();
            condition->conditions = std::move(conditions);
            return condition;
        }
    };

    template <typename ast_t>
    struct codegen_condition_all final : codegen_condition_logical<ast_t, codegen_condition_all<ast_t>>
    {
        using codegen_condition_logical<ast_t, codegen_condition_all>::conditions;

        static constexpr std::string_view type()
        {
            return "all";
        }

        bool match_ast(const ast_t& ast) const override
        {
            const auto predicate = [&](const std::shared_ptr<codegen_condition<ast_t>>& condition) {
                return condition && condition->match_ast(ast);
            };

            return std::all_of(conditions.begin(), conditions.end(), predicate);
        }
    };

    template <typename ast_t>
    struct codegen_condition_any final : codegen_condition_logical<ast_t, codegen_condition_any<ast_t>>
    {
        using codegen_condition_logical<ast_t, codegen_condition_any>::conditions;

        static constexpr std::string_view type()
        {
            return "any";
        }

        bool match_ast(const ast_t& ast) const override
        {
            const auto predicate = [&](const std::shared_ptr<codegen_condition<ast_t>>& condition) {
                return condition && condition->match_ast(ast);
            };

            return std::any_of(conditions.begin(), conditions.end(), predicate);
        }
    };

    template <typename ast_t>
    struct codegen_condition_none final : codegen_condition_logical<ast_t, codegen_condition_none<ast_t>>
    {
        using codegen_condition_logical<ast_t, codegen_condition_none>::conditions;

        static constexpr std::string_view type()
        {
            return "none";
        }

        bool match_ast(const ast_t& ast) const override
        {
            const auto predicate = [&](const std::shared_ptr<codegen_condition<ast_t>>& condition) {
                return condition && condition->match_ast(ast);
            };

            return std::none_of(conditions.begin(), conditions.end(), predicate);
        }
    };
}