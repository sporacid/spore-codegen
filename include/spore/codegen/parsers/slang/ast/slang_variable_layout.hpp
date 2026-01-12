#pragma once

#include "slang_stage.hpp"
#include "spore/codegen/parsers/slang/ast/slang_image_format.hpp"
#include "spore/codegen/parsers/slang/ast/slang_layout_unit.hpp"

namespace spore::codegen
{
    struct slang_variable_layout
    {
        slang_layout_unit unit = slang_layout_unit::none;
        slang_stage stage = slang_stage::none;
        slang_image_format image_format = slang_image_format::none;
        std::size_t offset = 0;
    };
}