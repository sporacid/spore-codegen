#pragma once

#include "spore/codegen/parsers/slang/ast/slang_attribute.hpp"
#include "spore/codegen/parsers/slang/ast/slang_variable.hpp"
#include "spore/codegen/parsers/slang/ast/slang_type_layout.hpp"

namespace spore::codegen
{
    struct slang_type
    {
        std::string name;
        std::vector<slang_variable> fields;
        std::vector<slang_type_layout> layouts;
        std::vector<slang_attribute> attributes;
    };
}