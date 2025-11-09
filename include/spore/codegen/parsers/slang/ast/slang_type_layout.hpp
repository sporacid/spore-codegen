#pragma once

#include "spore/codegen/parsers/slang/ast/slang_layout_unit.hpp"

namespace spore::codegen
{
    struct slang_type_layout
    {
        slang_layout_unit unit = slang_layout_unit::none;
        std::size_t size = 0;
        std::size_t stride = 0;
        std::size_t alignment = 0;
    };
}