#pragma once

#include "spore/codegen/parsers/slang/ast/slang_attribute.hpp"
#include "spore/codegen/parsers/slang/ast/slang_modifier.hpp"
#include "spore/codegen/parsers/slang/ast/slang_target.hpp"
#include "spore/codegen/parsers/slang/ast/slang_variable_layout.hpp"

#include <string>
#include <vector>

namespace spore::codegen
{
    struct slang_variable
    {
        std::string type;
        std::string name;
        slang_modifier modifiers = slang_modifier::none;
        std::vector<slang_target<slang_variable_layout>> targets;
        std::vector<slang_attribute> attributes;
    };
}