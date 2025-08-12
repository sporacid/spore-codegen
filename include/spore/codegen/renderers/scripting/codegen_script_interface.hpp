#pragma once

#include <ranges>
#include <string_view>

#include "nlohmann/json.hpp"

namespace spore::codegen
{
    struct codegen_script_interface
    {
        virtual ~codegen_script_interface() = default;
        [[nodiscard]] virtual bool can_invoke_function(std::string_view function_name) const = 0;
        [[nodiscard]] virtual bool invoke_function(std::string_view function_name, std::span<const nlohmann::json> params, nlohmann::json& result) = 0;
        virtual void for_each_function(const std::function<void(std::string_view)>&) = 0;
    };
}