#pragma once

namespace spore::codegen
{
    struct slang_type_layout
    {
        slang_layout_unit unit = slang_layout_unit::none;
        std::size_t size = 0;
        std::size_t alignment = 0;
    };
}