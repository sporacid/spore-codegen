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
        std::optional<slang_variable> output;
        std::vector<slang_variable> inputs;
        std::vector<slang_attribute> attributes;
    };
}