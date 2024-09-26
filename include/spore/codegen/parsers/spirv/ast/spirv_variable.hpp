#pragma once

#include <cstdint>
#include <string>

#include "spore/codegen/parsers/spirv/ast/spirv_type.hpp"

namespace spore::codegen
{
    struct spirv_variable
    {
        std::string name;
        std::size_t index = 0;
        spirv_type type;
    };
}