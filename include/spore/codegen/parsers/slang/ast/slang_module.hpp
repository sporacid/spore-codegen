#pragma once

#include "spore/codegen/parsers/slang/ast/slang_struct.hpp"

namespace spore::codegen
{
    struct slang_module
    {
        std::string path;
        std::vector<slang_struct> structs;
    };
}