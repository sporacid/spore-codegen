#pragma once

#include "nlohmann/json.hpp"

namespace spore::codegen
{
    template <typename cpp_value_t>
    struct cpp_has_attributes
    {
        nlohmann::json attributes;
    };
}