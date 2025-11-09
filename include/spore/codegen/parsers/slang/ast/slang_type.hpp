#pragma once

#include "spore/codegen/parsers/slang/ast/slang_attribute.hpp"
#include "spore/codegen/parsers/slang/ast/slang_target.hpp"
#include "spore/codegen/parsers/slang/ast/slang_type_layout.hpp"
#include "spore/codegen/parsers/slang/ast/slang_variable.hpp"

namespace spore::codegen
{
    struct slang_type
    {
        std::string name;
        std::vector<slang_variable> fields;
        std::vector<slang_target<slang_type_layout>> targets;
        std::vector<slang_attribute> attributes;
    };
}