#pragma once

#include <string>

#include "lua.hpp"
#include "nlohmann/json.hpp"

#include "spore/codegen/renderers/codegen_scripting_functions.hpp"

namespace spore::codegen
{
    struct codegen_scripting_functions_lua final : codegen_scripting_functions
    {
        [[nodiscard]] bool invoke(std::string_view function, const nlohmann::json& params, nlohmann::json& result) override
        {
            return false;
        }

        [[nodiscard]] bool can_invoke(std::string_view function, const nlohmann::json& params) const override
        {
            return false;
        }
    };
}