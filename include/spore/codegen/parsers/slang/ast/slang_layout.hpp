#pragma once

#include <cstddef>

namespace spore::codegen
{
    struct slang_layout
    {
        std::size_t space = 0;
        std::size_t binding = 0;
    };
}