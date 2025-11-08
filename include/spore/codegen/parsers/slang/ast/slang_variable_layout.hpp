#pragma once

#include "spore/codegen/parsers/slang/ast/slang_layout_unit.hpp"

namespace spore::codegen
{
    struct slang_variable_layout
    {
        slang_layout_unit unit = slang_layout_unit::none;
        std::size_t offset = 0;
    };
}