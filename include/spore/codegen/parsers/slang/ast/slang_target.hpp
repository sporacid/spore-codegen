#pragma once

#include <string>
#include <vector>

namespace spore::codegen
{
    template <typename layout_t>
    struct slang_target
    {
        std::string name;
        std::vector<layout_t> layouts;
    };
}