#pragma once

#include "spore/codegen/parsers/slang/ast/slang_attribute.hpp"
#include "spore/codegen/parsers/slang/ast/slang_type.hpp"
#include "spore/codegen/parsers/slang/ast/slang_variable_layout.hpp"

#include <optional>
#include <string>
#include <vector>

namespace spore::codegen
{
    struct slang_variable
    {
        // std::string type;
        std::string name;
        std::optional<slang_type> type;
        std::vector<slang_variable_layout> layouts;
        std::vector<slang_attribute> attributes;
    };
}