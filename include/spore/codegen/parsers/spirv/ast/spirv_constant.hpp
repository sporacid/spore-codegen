#pragma once

#include <string>
#include <vector>

#include "spore/codegen/parsers/spirv/ast/spirv_type.hpp"
#include "spore/codegen/parsers/spirv/ast/spirv_variable.hpp"

namespace spore::codegen
{
    struct spirv_constant
    {
        std::string name;
        std::size_t offset = 0;
        spirv_type type;
        std::vector<spirv_variable> variables;
    };
}