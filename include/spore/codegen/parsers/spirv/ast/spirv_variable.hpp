#pragma once

#include <cstdint>
#include <string>

#include "spore/codegen/parsers/spirv/ast/spirv_type.hpp"

namespace spore::codegen
{
    enum class spirv_variable_kind
    {
        none,
        input,
        output,
    };

    struct spirv_variable
    {
        spirv_variable_kind kind = spirv_variable_kind::none;
        std::string name;
        std::size_t index = 0;
        spirv_type type;
        // std::size_t size = 0;
        // std::size_t count = 0;
        // std::size_t format = 0;
    };
}