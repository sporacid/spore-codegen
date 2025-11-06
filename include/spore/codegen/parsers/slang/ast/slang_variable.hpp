#pragma once

#include "spore/codegen/parsers/slang/ast/slang_attribute.hpp"
#include "spore/codegen/parsers/slang/ast/slang_layout.hpp"

#include <optional>
#include <string>
#include <vector>

namespace spore::codegen
{
    struct slang_variable
    {
        std::string type;
        std::string name;
        std::vector<slang_attribute> attributes;
        std::optional<slang_layout> layout;
    };
}