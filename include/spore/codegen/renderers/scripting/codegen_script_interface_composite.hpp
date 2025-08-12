#pragma once

#include <concepts>
#include <memory>
#include <vector>

#include "spore/codegen/renderers/scripting/codegen_script_interface.hpp"

namespace spore::codegen
{
    struct codegen_script_interface_composite final : codegen_script_interface
    {
        std::vector<std::unique_ptr<codegen_script_interface>> script_interfaces;

        template <std::derived_from<codegen_script_interface>... script_interfaces_t>
        explicit codegen_script_interface_composite(std::unique_ptr<script_interfaces_t>&&... in_script_interfaces)
        {
            script_interfaces.reserve(sizeof...(script_interfaces_t));
            (script_interfaces.emplace_back(std::move(in_script_interfaces)), ...);
        }

        [[nodiscard]] bool invoke_function(const std::string_view function_name, const std::span<const nlohmann::json> params, nlohmann::json& result) override
        {
            const auto predicate = [&](const std::unique_ptr<codegen_script_interface>& script_interface) {
                return script_interface->can_invoke_function(function_name);
            };

            const auto it_script_interface = std::ranges::find_if(script_interfaces, predicate);
            if (it_script_interface != script_interfaces.end())
            {
                const std::unique_ptr<codegen_script_interface>& script_interface = *it_script_interface;
                return script_interface->invoke_function(function_name, params, result);
            }

            return false;
        }

        [[nodiscard]] bool can_invoke_function(const std::string_view function_name) const override
        {
            const auto predicate = [&](const std::unique_ptr<codegen_script_interface>& script_interface) {
                return script_interface->can_invoke_function(function_name);
            };

            return std::ranges::any_of(script_interfaces, predicate);
        }

        void for_each_function(const std::function<void(std::string_view)>& func) override
        {
            for (const std::unique_ptr<codegen_script_interface>& script_interface : script_interfaces)
            {
                script_interface->for_each_function(func);
            }
        }
    };
}