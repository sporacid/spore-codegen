#pragma once

#include "nlohmann/json.hpp"

namespace spore::codegen
{
    template <typename ast_value_t>
    struct ast_has_attributes
    {
        nlohmann::json attributes;
    };
}