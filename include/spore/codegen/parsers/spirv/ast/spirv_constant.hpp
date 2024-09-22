#pragma once

#include <string>
#include <vector>

#include "spore/codegen/parsers/spirv/ast/spirv_variable.hpp"

namespace spore::codegen
{
    struct spirv_constant
    {
        std::string name;
        std::string type;
        std::size_t offset = 0;
        std::vector<spirv_variable> variables;
    };
}