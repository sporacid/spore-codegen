#pragma once

#include <string>

#include "nlohmann/json.hpp"

namespace spore::codegen
{
    struct codegen_scripting_functions
    {
        virtual ~codegen_scripting_functions() = default;
        [[nodiscard]] virtual bool invoke(std::string_view function, const nlohmann::json& params, nlohmann::json& result) = 0;
        [[nodiscard]] virtual bool can_invoke(std::string_view function, const nlohmann::json& params) const = 0;
    };
}