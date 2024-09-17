#pragma once

#include <string>
#include <unordered_map>

#include "nlohmann/json.hpp"

namespace spore::codegen
{
    template <typename ast_t>
    struct codegen_condition
    {
        virtual ~codegen_condition() = default;
        virtual bool match_ast(const ast_t& ast) const = 0;
    };

    struct codegen_condition_params
    {
        std::string type;
        nlohmann::json value;
    };

    void from_json(const nlohmann::json& json, codegen_condition_params& value)
    {
        json["type"].get_to(value.type);
        json["value"].get_to(value.value);
    }

    template <typename ast_t>
    struct codegen_condition_factory
    {
        using func_t = std::function<std::shared_ptr<codegen_condition<ast_t>>(const nlohmann::json&)>;

        std::unordered_map<std::string, func_t> factory_map;

        static codegen_condition_factory& get()
        {
            static codegen_condition_factory instance;
            return instance;
        }

        template <typename condition_t>
        void register_condition()
        {
            register_condition(condition_t::type().data(), &condition_t::make_condition);
        }

        void register_condition(const std::string& type, const func_t& func)
        {
            factory_map.emplace(type, func);
        }

        [[nodiscard]] std::shared_ptr<codegen_condition<ast_t>> make_condition(const nlohmann::json& json) const
        {
            codegen_condition_params params;
            from_json(json, params);
            return make_condition(params);
        }

        [[nodiscard]] std::shared_ptr<codegen_condition<ast_t>> make_condition(const codegen_condition_params& params) const
        {
            const auto it = factory_map.find(params.type);
            return it != factory_map.end() ? it->second(params.value) : nullptr;
        }
    };

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
    struct codegen_condition_all : codegen_condition_logical<ast_t, codegen_condition_all<ast_t>>
    {
        static constexpr std::string_view type()
        {
            return "all";
        }

        bool match_ast(const ast_t& ast) const override
        {
            const auto predicate = [&](const std::shared_ptr<codegen_condition<ast_t>>& condition) {
                return condition && condition->matches_condition(ast);
            };

            return std::all_of(conditions.begin(), conditions.end(), predicate);
        }
    };

    template <typename ast_t>
    struct codegen_condition_any : codegen_condition_logical<ast_t, codegen_condition_any<ast_t>>
    {
        static constexpr std::string_view type()
        {
            return "any";
        }

        bool match_ast(const ast_t& ast) const override
        {
            const auto predicate = [&](const std::shared_ptr<codegen_condition<ast_t>>& condition) {
                return condition && condition->matches_condition(ast);
            };

            return std::any_of(conditions.begin(), conditions.end(), predicate);
        }
    };

    template <typename ast_t>
    struct codegen_condition_none : codegen_condition_logical<ast_t, codegen_condition_none<ast_t>>
    {
        static constexpr std::string_view type()
        {
            return "none";
        }

        bool match_ast(const ast_t& ast) const override
        {
            const auto predicate = [&](const std::shared_ptr<codegen_condition<ast_t>>& condition) {
                return condition && condition->matches_condition(ast);
            };

            return std::none_of(conditions.begin(), conditions.end(), predicate);
        }
    };
}