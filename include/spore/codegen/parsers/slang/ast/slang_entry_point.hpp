#pragma once

#include "spore/codegen/parsers/slang/ast/slang_attribute.hpp"
#include "spore/codegen/parsers/slang/ast/slang_variable.hpp"

#include <optional>
#include <string>
#include <vector>

namespace spore::codegen
{
    struct slang_entry_point
    {
        std::string name;
        slang_modifier modifiers = slang_modifier::none;
        slang_stage stage = slang_stage::none;
        std::optional<std::string> return_type;
        std::vector<slang_variable> arguments;
        std::vector<slang_attribute> attributes;
    };
}