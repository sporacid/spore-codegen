#pragma once

#include <string>
#include <unordered_map>

#include "nlohmann/json.hpp"

#include "spore/codegen/conditions/codegen_condition.hpp"

namespace spore::codegen
{
    struct codegen_condition_params
    {
        std::string type;
        nlohmann::json value;
    };

    inline void from_json(const nlohmann::json& json, codegen_condition_params& value)
    {
        json["type"].get_to(value.type);
        json["value"].get_to(value.value);
    }

    template <typename ast_t>
    struct codegen_condition_factory
    {
        static codegen_condition_factory& get()
        {
            static codegen_condition_factory instance;
            return instance;
        }

        template <typename condition_t>
        void register_condition()
        {
            factory_map.emplace(condition_t::type().data(), &condition_t::make_condition);
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

      private:
        using func_t = std::function<std::shared_ptr<codegen_condition<ast_t>>(const nlohmann::json&)>;
        std::unordered_map<std::string, func_t> factory_map;
    };
}