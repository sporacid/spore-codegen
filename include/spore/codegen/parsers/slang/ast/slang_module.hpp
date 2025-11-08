#pragma once

#include "spore/codegen/parsers/slang/ast/slang_entry_point.hpp"
#include "spore/codegen/parsers/slang/ast/slang_type.hpp"
#include "spore/codegen/parsers/slang/ast/slang_variable.hpp"

namespace spore::codegen
{
    struct slang_module
    {
        std::string path;
        std::vector<slang_type> types;
        std::vector<slang_variable> variables;
        std::vector<slang_entry_point> entry_points;
    };
}